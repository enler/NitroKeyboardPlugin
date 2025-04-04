# 0. 前言

本插件的设计目标是为了让nds汉化游戏能更好地输入中文而设计的，  
在汉化游戏中接入本插件后，可以在游戏内呼出键盘，直接编辑内存中的字符串。  
这个做法的好处就是可以减少对游戏自身的romhack，并且可以突破各种限制，比如雷顿教授那种纯手写输入的游戏，无法按照传统的方案为其添加中文输入功能。  

由于nds游戏会使用各种各样的文本编码，并且nds主机也没有统一的系统字库，因此需要在接入的时候编写相关的代码来进行适配。  

在开始之前，先安装最新的devkitpro，确保libnds的版本是2.0.0，然后再安装python3，根据script目录下的requirements.txt安装必要的库。  

接下来将介绍接入本插件的方法，注意，如果不了解nds的汉化跟逆向工程，阅读本文可能会感到费解。  

# 1. 项目目录结构

- **overlay目录**：键盘插件的核心代码实现  
- **overlay_ldr目录**：用于加载overlay的辅助插件，需要注入到nds的arm9.bin中，并通过hook来调用  
- **script目录**：存放了一些实用脚本，比如rom_analyzer.py可以帮助你分析rom，寻找键盘插件所需的NitroSDK函数  
- **example目录**：里面有些示例，我在后文会简单讲解  
- **common目录**：存放共用的头文件，以及NitroSDK的函数定义，rom_analyzer.py生成的config.mk跟symbols.ld也是放在这里的  
- **resource目录**：目前只有一个keys.tex，这是键盘插件的UI贴图  

# 2. 一些背景知识

## 关于nds游戏的内存布局

对于基于任天堂nitrosdk开发的游戏，大致的内存布局是这样的（真实情况当然更复杂）：  

```
0x2000000 arm9.bin
          overlay
          arena
0x23E0000 arm7.bin
```

arena这块内存一般作为游戏的堆，用于动态分配内存，可以通过修改arena的指针，  
将arena的起始地址向后移动，这样就开辟了一段空闲的内存出来，也可以理解为把原来的overlay的空间扩大了。  
键盘插件默认就放在这个位置上，当然，如果你发现内存中有其他地方可以存放键盘插件，那也可以不用这个方法。  

然后，对于ndsi增强游戏，在ndsi模式下，大致的内存布局是这样的：  

```
0x2000000 arm9.bin
          overlay
          arm9i.bin <- 加载到了原本arena的起始区域上
          arena_twl <- ndsi模式下，arena的起始地址移动到了arm9i.bin的后面
0x2FE0000 arm7.bin
```

ndsi增强游戏都有2个arena指针，一个是nds模式用，一个是ndsi模式用(arena_twl)，  
此时需要同时修改2个arena指针，并且2个指针均以arena_twl为基础向后移动，才能确保键盘插件在ndsi模式下不会被arm9i覆盖。  

修改2个arena指针之后，内存布局像这样：

**nds模式**
```
0x2000000 arm9.bin
          overlay
          保留给arm9i的空间，但是nds模式下无数据
          键盘插件
          arena
0x23E0000 arm7.bin
```

**ndsi模式**
```
0x2000000 arm9.bin
          overlay
          arm9i.bin
          键盘插件
          arena_twl
0x2FE0000 arm7.bin
```

相信你也注意到了，这样修改arena的指针，虽然可以确保不会跟arm9i.bin冲突了，  
但是在nds模式下，用于存放arm9i的那块内存被浪费了，关于这个问题，我会在后面给出一些思路。  

补充一个例外的情况，就是宝可梦黑白2，  
黑白2的ndsi模式下的内存布局大致如下：

```
0x2000000 arm9.bin
          overlay
          无用空间 <-原本nds模式下的arena到arm9i之间空出了5M内存
0x2700000 arm9i.bin
          arena_twl
0x2FE0000 arm7.bin
```

这个游戏的arm9i并没有加载在overlay区域的后面，而是加载到了0x2700000这个地址上，  
不然在ndsi模式下，宝可梦黑白2汉化版的中文输入法将被arm9i.bin覆盖，导致奔溃，这也使得黑白2在ndsi模式下有接近5M的内存没有被游戏利用。

## 关于overlay

nds的overlay是一种动态加载的模块，但是它不具备动态链接之类的特性，只能被加载到指定的内存空间上，  
键盘插件也是以overlay的形式来加载的，一般直接调用`FS_LoadOverlay`即可加载overlay，该函数的调用流程如下：

1. 根据overlay的id读取overlay_table
2. 获取地址等信息
3. 加载overlay
4. 如果有压缩先解压
5. 调用overlay的静态初始化函数

可以注意到，只要调用了`FS_LoadOverlay`去加载overlay，  
后面就会直接运行overlay里的初始化代码，而不需要在加载后另行调用。  
而包括地址、大小、初始化函数的地址等信息，都存储在overlay_table里，  
因此我们只需要往overlay_table里添加新的项，接着在arm9.bin里的某个地方hook，  
在hook的地方调用`FS_LoadOverlay`，就可以加载我们自己的overlay了。  
overlay_ldr正是调用了`FS_LoadOverlay`来加载键盘插件的overlay。

不过，事情显然并不会这么顺利，首先并不是所有游戏都使用了overlay，  
对于没有使用overlay的游戏，并不会链接FS_LoadOverlay在内的overlay函数，  
其次就算是使用了overlay，它也有可能用其他方式加载overlay，  
sdk中还提供了`FS_LoadOverlayImage`这种只加载overlay的接口，  
宝可梦信长就是自己实现了一套加载overlay的方案。  
还有就是sdk5的`FS_LoadOverlayInfo`还会检测overlay_id是否超过上限，  
需要事先patch掉这个地方，才能顺利加载追加的overlay。  
另外，overlay还有hmac校验，虽然卡带版游戏并不校验，  
但在dsiware、download play中是会校验的，而且overlay的hmac是存在arm9.bin里的，  
修改起来也很麻烦，最好的方法还是patch掉`FS_StartOverlay`这个函数，让它不去校验。  

好在nds除了overlay，还有一个autoload机制，也可以帮助我们加载键盘插件。  

## 关于autoload

在arm9.bin启动的时候，它会先进行各种初始化设置，然后解压缩自身，解压完成后，  
它会把附加在arm9.bin后面的section复制到其他的指定内存上，这就是autoload。  
一般至少会放一个itcm的section在尾部，而且附加的section数量并没有限制。  
所以我们也可以利用这一点，把overlay附加到arm9.bin的后面，通过autoload加载到指定内存上。  
arm9i也是靠autoload来加载到目标内存上的，  
对于dsi有所了解的人，可能会记得arm9i.bin是加载到0x2400000上的，  
这是对的，但不全对，因为加载到0x2400000之后，  
arm9.bin的初始化代码还会把arm9i再复制一次到指定内存上。  

使用autoload加载主要就一个问题，那就是arm9.bin会被扩大，在某些时候可能不方便，  
比如一些工具可能无法替换掉扩容后的arm9.bin。

在理解一些基本信息之后，我们接下来将正式开始接入了

# 3. 初步配置

首先，运行script目录下rom_analyzer.py分析rom。  
命令是  
```
python script/rom_analyzer.py <nds_rom_path> > log.txt
```  

在运行完成之后，会生成config.mk跟symbols.ld，将它们复制到common目录下，rom_analyzer.py输出的最好也存起来，后面要用到。  

然后打开config.mk，  
`OVERLAY_ADDR`就是overlay的地址，如果你找到了其他合适存放键盘插件的内存空间，可以修改它。  
`OVERLAY_LDR_ADDR`是overlay_ldr的地址，位于arm9.bin的头部。  

arm9头部的2048字节是secure area，其中随机插入了syscalls函数，但大部分还是无用数据，  
rom_analyzer.py会寻找其中可能无用的区域来作为overlay_ldr的加载地址。  

注意，很多游戏的反烧录补丁也会使用这个区域，rom_analyzer.py并不能识别到这种情况，请进行逆向工程，判断是否存在反烧录补丁。  
如果你在arm9.bin找到了合适的空间，也可以修改这个地址。  

`INJECT_OVERLAY_ID`这个跟我们的hook方案有关了，先来介绍一下注入到arm9.bin的overlay_ldr该如何启动。  

上面说到了要进行hook，下面就是我推荐的hook方案。  
我推荐的方案是替换掉游戏中第一个被载入的overlay的静态初始化函数，  
通过修改overlay_table，将第一个被载入的overlay的静态初始化函数替换成overlay_ldr中的函数，  
此时当第一个overlay被加载之后，在调用静态初始化函数的时候就会跳转到overlay_ldr中了，  
然后overlay_ldr再去调用一个overlay的静态初始化函数，完成闭环。  

原本的流程是  
```
游戏调用FS_LoadOverlay加载第一个overlay ->  
调用第一个overlay的静态初始化函数  
```

修改后的流程是  
```
游戏调用FS_LoadOverlay加载第一个overlay ->  
调用overlay_ldr ->调用FS_LoadOverlay加载键盘插件 ->   
调用第一个overlay的静态初始化函数  
```

`INJECT_OVERLAY_ID`这个变量正是第一个载入内存的overlay的id，  
那么该如何确认第一个载入内存的overlay id呢。  

请打开symbols.ld，找到`FS_LoadOverlay`，  
在游戏里对着`FS_LoadOverlay`下断点，然后在游戏重启之后等到断点被触发，  
查看r1寄存器的值，就是第一个载入内存的overlay的id，如果没有，应该也会有`FS_LoadOverlayImage`等函数，  
不过这些函数的参数中并不能直观地看出overlay的id，请进行更多的逆向工程来判断。  

虽然已经确认了第一个overlay的id，但overlay_ldr的配置还没结束，请查看之前运行rom_analyzer.py的输出。  
上面的流程可以看出，overlay_ldr除了要加载键盘的overlay，还需要调用第一个overlay的静态初始化函数才能完成逻辑闭环。  
我们先找到输出中的overlay的有关信息，然后找到第一个overlay的id对应的那一行。  
可以看到最后2列包括了`"static_init_begin" "static_init_end"`这2个字段，这就是静态初始化函数列表的起始地址跟结束地址，  
接着将地址以如下格式加入到symbols.ld中，注意，是加入到symbols.ld。  

```
Orig_OverlayStaticInitBegin = <static_init_begin>;  
Orig_OverlayStaticInitEnd = <static_init_end>;  
```

至此，overlay_ldr就算配置完成了。  

这是我个人最推荐的加载并启动overlay的方法，这个方法很干净，  
没有修改arm9中的代码，只需要确保在arm9中找到一处无用的地方就行。  

不过，上面也说过了如果游戏没有overlay或者无法调用`FS_LoadOverlay`，  
那么overlay_ldr就没有用武之地了，相关的配置也可以无视。  

# 4. 适配游戏

我们先来看看example下的项目吧  
首先是assets目录，这个目录基本上就存了一个码表  
common目录就是配置文件跟符号文件  
关键在于overlay，在每个overlay/src目录下，都有一个`keyboard_game_interface.c`的源文件，  
适配具体游戏的核心逻辑就放在这里  

最后是rom_nitrofs，这个目录下存放的就是键盘插件所需要的资源文件，要打包到游戏的文件系统里，  
`keyboard/keys.tex`是键盘的UI贴图，跟根目录下的resource里的是一样的，  
`keyboard/pinyin_table.bin`是拼音表文件，用于实现拼音输入。  

在雷顿的目录下还有一个`keyboard/font.bin`，这是一个单独的字库文件，script下有工具可以生成。  
此外，在雷顿的目录下还有一个`.gds`文件，这个文件存了谜题的答案，为了演示，我改成了中文答案，因此也放在这里了。  

script下有2个工具用于生成需要的文件  
分别是`create_pinyin_table.py`跟`create_font.py`，  
`create_pinyin_table`用于生成拼音表，命令是  
```bash
python script/create_pinyin_table.py <charmap_path>
```  
`create_font`用于生成字库，命令是  
```bash
python script/create_font.py <charmap_path> <font_path>
```  
字体推荐xp宋，懂得都懂。  

工具里支持的编码表是比较传统的格式  
```
4E00=一
```  
就像这样，可以通过修改`script/common.py`中的正则表达式让工具能读取其他格式的编码表。  

键盘插件使用的是双字节编码，对于单双混合类型的编码，  
键盘插件统一视作双字节编码处理，由此确实会带来一些问题，在适配的时候需要注意。  
另外对于3字节的utf8编码，请转换成等价的utf16编码再处理，具体细节可以看dq5，dq5就是utf8编码。  

可能也有人注意到了，好几个示例的rom_nitrofs里是没有`font.bin`的，  
是的，这几个项目并没有用到`font.bin`，而是用了汉化游戏自身的字库，这些后面都会进一步说明。  

我们再来看看`keyboard_game_interface.c`  
拉到最下面，可以看到  
```c
KeyboardGameInterface * GetKeyboardGameInterface() {
    static KeyboardGameInterface gameInterface = {
        .Alloc = Alloc,
        .Free = Free,
        .OnOverlayLoaded = OnOverlayLoaded,
        .ShouldShowKeyboard = ShouldShowKeyboard,
        .GetMaxInputLength = GetMaxInputLength,
        .LoadGlyph = LoadGlyph,
        .KeycodeToChar = KeycodeToChar,
        .CanContinueInput = CanContinueInput,
        .OnInputFinished = OnInputFinished
    };
    return &gameInterface;
}
```  
这就是用户需要进行适配的接口了，一共有9个函数，键盘插件会根据情况调用这些接口。  

首先是  
```c
void * Alloc(u32 size);
void Free(void *ptr);
```  
这2个函数，是分配内存的函数，键盘插件会一次性申请24KB的内存用于插件运行。  
在nds上，游戏拿到arena内存的地址之后，会用这个地址去初始化堆内存分配器，  
任天堂分别在NitroSDK跟NitroSystem各提供了一个内存分配器，对应的内存分配函数就是  
NitroSDK的  
```c
void * OS_AllocFromHeap(s32 id, s32 heap, u32 size);
void OS_FreeToHeap(s32 id, s32 heap, void *ptr);
```  
NitroSystem的  
```c
void *FndAllocFromExpHeapEx(void *heapHandle, u32 size, s32 flag);
void FndFreeToExpHeap(void *heapHandle, void *ptr);
```  
example中，dq5跟宝可梦信长使用了NitroSystem的内存分配器，雷顿使用了NitroSDK的内存分配器，  
而宝可梦心金比较特殊，因为心金这款游戏拿到了arena之后，并没有全部用掉，还留下了一部分，  
所以心金可以直接拿当前arena的指针。  

宝可梦信长的话，它是ndsi增强游戏，会判断当前是否是ndsi(twl)模式，如果不是的话，就会把arm9i的那块内存拿出来，进行废物利用。  
具体游戏中用了哪种分配器，请下断点判断，另外，NitroSystem的内存函数的第一个参数是堆的起始地址，  
这个地址是会因为arena的范围变化的，不要直接复制下断点时看到的参数，请进行逆向，想办法间接获取。  
有的游戏可能这2个内存分配器都不使用，而是使用自定义的，请根据逆向的实际情况加入函数。  

接着是  
```c
void OnOverlayLoaded();
```  
这个函数是overlay被加载后，马上就会被调用的函数，  
可以在这个函数里进行一些初始化操作，包括进行inline hook等。  
基本上所有的示例都初始化了一个bpp转换表，这是用于后面转换字模用的，  
除此之外，也对一些函数进行了hook，主要是对起名字时的初始化跟退出函数进行hook，  
在hook函数中获取到的相关变量，既可用于判断是否应该弹出键盘，也可用于编辑相关的名字变量等。  

然后是  
```c
bool ShouldShowKeyboard();
```  
该函数用于判断是否应该弹出键盘，弹出键盘的组合按键也是放在这里的，代码基本上是判断按键+变量。  

再然后是  
```c
int GetMaxInputLength();
```  
这个函数返回最多可以输入的字符数，注意是字符数，不是字节数，  
如果是双字节编码，名字用了10字节存储，但实际上只能存5个字符，这里返回5。  
如果是单双混合编码，全是单字节情况下能取10个字符，在全是双字节的情况下能用5个字符，在这里要返回最多可以输入的字符数，也就是10。  

在示例中，基本都不是硬编码返回字符数，而是间接获取字符数，  
这是因为游戏里的输入界面可能会在不同的地方弹出，能输入的字符数也不一样，  
比如宝可梦心金，在给宝可梦取名是5个字符，在给电脑的盒子重命名的时候，  
最多可以输入8个字符，所以一般来说，还是间接获取更好。  

重头戏来了  
```c
bool LoadGlyph(u16 charCode, u8 *output, int *advance)
```  
这个函数用于获取字模，获取成功后返回true，获取后的字模存入output里，字符间距存入advance里，也可以存宽度，但最好存间距  
存入output里的字模格式必须为16x16 2bpp低位在前的格式，不确定的话，可以看看ct2的vb2bpp  
几个示例里，这个函数的实现都不完全一样  
宝可梦心金是直接读取rom中游戏自身的字库文件，然后转换  
宝可梦信长是调用了游戏的函数获取字模  
dq5是读取了内存里的nftr字库  
只有雷顿用了脚本生成的font.bin  

这个函数的实现很重要，键盘插件里的候选汉字跟输出栏文字都需要通过这个函数接口获取字模来显示。  
如果不确定该怎么实现，请直接参考雷顿的示例。  

接着是这个  
```c
bool KeycodeToChar(u16 keycode, u16 *output);
```  
这个函数用于转换keycode到游戏自身的编码，keycode就是键盘插件内部的按键值，  
实际的取值跟utf16编码是一样的，为了确保键盘插件输入的字母、数字、符号也能正常在游戏中显示，  
需要在这个函数中进行转换，如果游戏本身已经是utf16编码了（或者等价的编码，比如utf8），那么就可以直接使用。  
像dq5就是utf8编码，所以这个函数的实现就很简单，  
只需要判断哪些字符游戏字库里没有，然后返回false就行（虽然dq5汉化有'>'却没有'<'是个挺奇怪的事情）  

其他编码的情况，可以用script的`create_keycode_conv_table.py`来生成一个转换表，命令是  
```bash
python script/create_keycode_conv_table.py <charmap_path>
```  
会生成一个c语言的数组  
```c
const KeycodeConvItem gKeycodeConvTable[] = {...}
```  
像这样  
然后你就可以在转换的时候查表转，但是，这个工具并不会为数字、大小写字母生成转换表，  
因为一般来说，数字跟大小写字母都是作为连续的编码存储的，转换的时候，直接做加减法转换即可，比查表快。  

上面提到的单双字节混合的编码，可能会带来一些问题，如果是单双字节混合的编码，下面这个函数就需要重视起来了。  
```c
bool CanContinueInput(u16 *inputText, int length, u16 nextChar)
```  
这个函数的作用，用于判断是否允许用户继续输入，  
对于双字节编码来说，这个函数基本可以无脑返回true，因为字符数的判断在键盘插件里就已经完成了。  
但是单双字节混合的情况下，就需要在这个函数里判断是否会越界了，  
比如存在10字节的限制，inputText已经有9个单字节字符了，再输入一个双字节字符就越界了。  
在几个例子中，雷顿是单双字节混合的，但雷顿存储输入的答案是存储在几个不同的结构体里的，所以并不需要额外判断。  

最后一个要实现的接口就是  
```c
void OnInputFinished(u16 *inputText, int length, bool isCanceled);
```  
当用户按下回车键或者退格键返回的时候，将会调用这个函数，  
inputText就是用户输入的字符，length就是字符数，isCanceled为true时，表示用户取消输入。  
在这个函数内，就需要将inputText复制到游戏内存中的字符串变量了，并且在复制完成后，还要更新游戏画面，  
在宝可梦心金中，是调用了几个函数更新，在dq5中是设置了一个flag，雷顿也是设置了flag。  
另外像dq5这样的，还需要将utf16编码转成utf8编码进行存储。  

在某些情况下，还需要恢复一些IO寄存器，键盘插件在启动之后，为了绘制键盘，  
备份了4KB显存跟一部分IO寄存器，在退出后恢复，但有2个寄存器是只写的，无法备份，就是`REG_BG0HOFS`跟`REG_BG0VOFS`，  
适配的时候，如果这2个寄存器的值不为0，需要在这里编写代码恢复，  
宝可梦心金就有  
```c
int bg0H = *(int *)(namingContext + 0x468);
int bg0V = *(int *)(namingContext + 0x46C);
REG_BG0HOFS = 512 + bg0H;
REG_BG0VOFS = 512 + bg0V;
```  
这样的代码

# 5. 打包

在配置好了overlay_ldr跟适配好游戏之后，就可以将文件打包进游戏里了。  
本工程仅提供了注入arm9跟overlay_table的示例代码，并没有提供完整的打包流程，  
因为现在的打包方式挺多的，按照各自的喜好来即可。  

具体的patch代码都在`script/patch_util.py`里，具体的调用示例在`example`的`patch.py`里。  
基本上的流程就是用`ndspy`库读取rom，读取`arm9.bin`跟`overlay_table`，  
然后往`arm9.bin`写入`overlay_ldr`，往`overlay_table`添加新的overlay信息，  
修改第一个overlay的静态初始化函数地址以及修改arena的指针，  
最后保存`arm9.bin`跟`overlay_table`。  

宝可梦心金跟dq5的`patch.py`的实现都是一样的，  
宝可梦信长主要是因为它是ndsi增强游戏，多修改了一个twl下的arena指针。  
雷顿教授的情况则是它只有一个overlay，并且不在游戏启动时加载，所以虽然用了overlay的方案，  
但是雷顿的`overlay_ldr`则是需要靠hook跳转来执行，为了进行hook，雷顿的`patch.py`还调用了`armips`。  

# 6. 实战建议

## 逆向的一些思路
接入键盘最复杂的还是要想办法逆向跟游戏输入系统相关的函数跟变量地址  
因为需要hook相关函数，才能想办法确认当前状态，并且访问相关变量，  
要知道内存里名字字符串存在哪儿，才能在键盘退出的时候，把字符串复制到目标内存上。  
逆向的时候，可以在游戏里输入几个文字，比如'12345'，然后内存里去搜索12345，  
这样就可以找到字符串存在哪里，当然有时候可能没那么顺利，但大致思路是这样的，  
像雷顿，每个字符储存在了不同的地址上，最后我是靠寻找字库相关的函数倒回去看，才定位了内存  
接着可以对这块内存下写断点，在输入新的文字的时候，应该会触发断点，可以从这个地方开始逆向，  
寻找游戏输入系统相关的函数。

## 关于hook的解决方案
一些破解人员可能习惯于用armips做各种静态修改跟hook，  
不过在这里还是推荐使用键盘插件里提供的方法对代码进行hook。  
键盘插件附带了一个hook.c，可以在程序运行阶段，用它进行inline hook  

用于hook ARM函数的  
`void HookFunction(HookARMEntry *entry);`  

用于hook thumb函数的  
`void SetupHookMemory(void *memory, u32 size);`  
`void HookFunctionThumb(HookThumbEntry *entry);`  

具体的用法是这样  
```c
void (*Orig_foo)(); // 定义原函数的指针

void Hook_foo() {
    // do something
    Orig_foo();
}

void InstallHook() {
    static HookARMEntry entry = {
        .functionAddr = (void *)0x02468ACE,      // 在结构体里填写函数地址
        .origFunctionRef = (void **)&Orig_foo,   // 原函数指针的引用，
        .hookFunction = Hook_foo                 // hook的函数
    };

    HookFunction(&entry); // hook arm的函数这样写就行

    // 对于thumb的函数，参考以下代码
    static u8 hookMem[64];
    SetupHookMemory(hookMem, sizeof(hookMem)); // hook thumb的函数，需要划一块内存，
                                               // 用于备份指令，并生成跳转回去的指令，
                                               // 一个函数差不多用掉20到30字节

    static HookThumbEntry entry = {
        .functionAddr = (void *)0x02468ACE,
        .origFunctionRef = (void**)&Orig_foo,
        .hookFunction = Hook_foo
    };
    HookFunctionThumb(&entry);
}
```

hook.c会备份目标地址原有的指令，然后生成跳转到hook的指令覆盖掉原有的指令，  
并且在备份的指令后面，生成跳转回去的指令  

如果要对overlay里的函数进行hook，可以尝试hook FS_LoadOverlay，  
然后再对其中的函数进行hook。  

hook.c里还有这个函数  
```
void ForceMakingBranchLink(void *origAddr, void *targetAddr);`  
```  

这个函数可以将origAddr地址上的指令修改成跳转到targetAddr的bl(x)指令，支持arm、thumb，  
具体生成什么指令，取决于2个地址的最后一位，  
如果都为0，就是修改成arm -> arm的bl指令，如果都为1，就是修改成thumb -> thumb的bl指令  

## 关于调用游戏自身的函数
如果调用游戏里的函数，除了要在symbols.ld里添加函数地址，  
还要注意函数是否是thumb，如果是thumb，在symbols里定义的时候，要带上|1  
就比如  
```
foo = 0x02072A4C|1;  
```  
此外还要在代码里这样进行定义  
```c
#define IMPORT __attribute__((naked))

IMPORT void foo() {} // 注意，这里是花括号
```

这样链接的时候，才会当成一个thumb函数去链接，  
gcc在链接的时候，默认都是arm函数，由于键盘插件中的代码都是编译成thumb的，  
经过这样的定义后，gcc的链接器才会认为这个外部函数是一个thumb函数，才能正常链接。  

## 关于调用devkitpro里的函数
键盘插件为了尽可能减少不必要的链接，  
在overlay/linker.ld里显式地定义了目前所需的devkitpro里的静态库函数的section，  
如果你要调用strcmp这样的没有显式定义在linker.ld的函数时，  
在链接的时候，它会出现这样的错误  
`'strcmp' referenced in section '.text' of ... : defined in discarded section '.text' of ... libc.a(libc_a-strcmp.o)`  
这种错误就是没有在linker.ld里显式定义导致的，  
`*libc.a:libc_a-strcmp.o(.text)`  
需要在linker.ld里添加这么一行才行。  
我也尝试过用gcc自带的--gc-section，让链接器只保留必要的section，  
但达不到我想要的效果，还是会链接一些不必要的代码。  

## 提高早期游戏的兼容性
对于早期的游戏，主要是sdk2跟部分sdk3的游戏，  
不一定能顺利地呼出键盘，需要另外的方法接管主线程  
只需要添加一个`SVC_WaitVBlankIntr_Caller`的符号进去即可。  
在`SVC_WaitVBlankIntr`这个函数上下断点，  
然后将调用这个函数时的lr寄存器的值，  
作为`SVC_WaitVBlankIntr_Caller`的值，添加到symbols.ld的后面即可。  

## autoload的补充说明
如果没有overlay，只能用autoload的话，可以参考`script/patch_util.py`里的add_overlay_as_section  
调用这个函数之后，再调用save_arm9_binary就可以获得一个overlay附加到尾部的arm9.bin了  
hook的话，请至少在`OS_InitThread`调用完成之后，找一个地方进行hook，然后跳到overlay的起始地址就行，overlay的起始地址就是overlay的执行入口。  

## 一些关键接口未找到
除了`FS_LoadOverlay`，其他的几个FS接口都是最基础的接口，一般来说，不会出现找不到的情况，  
如果出现找不到的情况，请从其他几个已找到的FS接口入手，进行逆向工程去查找。  
我之前在测试`rom_analyzer.py`的时候，确实发现有个别游戏不链接`TP_GetCalibratedPoint`，  
`TP_GetCalibratedPoint`这个函数是用于计算校正后的触摸屏的位置的。  
经过研究发现，是这种游戏压根没用到触摸屏，  
完全不用触摸屏的情况相当少见，要是遇到这种情况，可以考虑为键盘插件增加基于按键的操作逻辑。  
另外，为这种游戏启用触摸屏也并不困难，因为触摸屏的驱动都在arm7一侧，  
arm9只不过是把arm7发过来的屏幕位置结合用户对于屏幕的校正进行计算而已，  
只需要实现NitroSDK中的tp.c中的几个核心函数即可，  
分别是`TP_Init`、`TP_GetUserInfo`、`TP_SetCalibrateParam`  
以及上面提到的`TP_GetCalibratedPoint`。  
还有，像DQ5虽然链接了这个函数，  
但没有调用TP_GetUserInfo跟TP_SetCalibrateParam来设置用户的校正数据，  
所以在适配的时候特意加上了，确保`TP_GetCalibratedPoint`能正常工作。


