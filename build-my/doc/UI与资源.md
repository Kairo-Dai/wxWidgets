# wxWidgets XRC：UI 布局与资源（XML / 字体 / 图片 / 打包）

本文基于当前 wxWidgets 源码与自带示例进行整理，说明：

- wxWidgets 如何用 XML（XRC）描述 UI 布局  
- XML 中如何引用字体、图片等资源  
- UI 资源在工程中的几种打包方式

相关参考代码（可在仓库中查看）：

- 总体介绍：`docs/doxygen/overviews/xrc.h`  
- XRC 格式细节：`docs/doxygen/overviews/xrc_format.h`  
- 运行时加载逻辑：`src/xrc/xmlres.cpp`、`interface/wx/xrc/xmlres.h`  
- 示例 XRC：`samples/xrc/rc/*.xrc`，`samples/propgrid/propgrid.xrc`

---

## 1. XRC 基本概念

XRC（XML Resource）是 wxWidgets 自带的 XML 资源系统，用来描述：

- 窗口：`wxFrame` / `wxDialog` / `wxPanel` 等  
- 菜单栏、工具栏、菜单项  
- 各种控件及布局（sizer）  
- 资源属性：字体、位图、图标、加速键等

特点：

- UI 从 C++ 代码中分离为 XML 文件（`.xrc`），改布局无需重新编译程序  
- UI 定义跨平台，利用 sizer 做自适应布局  
- 可打包成单个压缩资源文件（`.xrs` / `.zip`），或编译进可执行文件

应用启动时的典型用法（参考 `docs/doxygen/overviews/xrc.h`）：

```cpp
#include "wx/xrc/xmlres.h"

bool MyApp::OnInit()
{
    wxXmlResource::Get()->InitAllHandlers();   // 注册所有标准 XRC 控件 handler

    // 加载 XRC 文件，支持通配符与打包文件
    wxXmlResource::Get()->Load("resource.xrc");
    wxXmlResource::Get()->LoadAllFiles("rc");  // 加载目录下所有 *.xrc

    // 创建/加载窗口
    wxFrame* frame = wxXmlResource::Get()->LoadFrame(nullptr, "main_frame");
    frame->Show();
    return true;
}
```

---

## 2. 用 XML 描述 UI 布局

### 2.1 XRC 文件结构

每个 XRC 文件的根元素为 `<resource>`，包含若干顶层 `<object>`：

```xml
<?xml version="1.0"?>
<resource xmlns="http://www.wxwidgets.org/wxxrc" version="2.5.3.0">
  <object class="wxDialog" name="SimpleDialog">
    ...
  </object>
</resource>
```

- `version`：XRC 格式版本，建议始终使用库当前文档建议的最新版本（源码中示例为 `2.5.3.0`）。  
- 根节点的子元素是“顶层对象”，必须有唯一的 `name`，用于运行时加载。

### 2.2 `<object>`：控件/窗口的定义

`<object>` 通常对应一个 wxWidgets 对象实例（窗口、控件、sizer 等）：

```xml
<object class="wxDialog" name="my_dialog">
  <title>标题</title>
  <centered>1</centered>
  ...
</object>
```

关键属性：

- `class`：必填，对应 wxWidgets 的类名，例如 `wxFrame` / `wxDialog` / `wxButton` 等。  
- `name`：逻辑名称/ID：
  - `LoadDialog(&dlg, parent, "my_dialog")` 等 API 用它来定位对象  
  - 若是窗口，`wxWindow::GetName()` 会被设为该值  
  - 同时参与计算窗口/菜单项的数值 ID（`XRCID("name")`）
- `subclass`（可选）：使用自定义派生类替代 `class` 指定的类，用于扩展控件。

子元素有两类：

- “属性”（property）：例如 `<label>`、`<style>`、`<size>`、`<font>` 等  
- “子对象”：子窗口、sizer、sizeritem、notebook 页等，都是嵌套的 `<object>`

常见示例（节选自 `docs/doxygen/overviews/xrc.h`）：

```xml
<object class="wxDialog" name="SimpleDialog">
  <title>Simple dialog</title>
  <object class="wxBoxSizer">
    <orient>wxVERTICAL</orient>
    <object class="sizeritem">
      <object class="wxTextCtrl" name="text"/>
      <option>1</option>
      <flag>wxALL|wxEXPAND</flag>
      <border>10</border>
    </object>
    ...
  </object>
</object>
```

### 2.3 布局：sizer 与 sizeritem

布局依靠 sizer 系统，在 XRC 中也用 `<object>` 表达：

- 常用 sizer 类：
  - `wxBoxSizer` / `wxStaticBoxSizer`  
  - `wxGridSizer` / `wxFlexGridSizer` / `wxGridBagSizer`  
  - `wxWrapSizer` 等
- 子元素 `<object class="sizeritem">` 用来描述每个子控件/子 sizer 的布局参数：
  - `<option>`：权重  
  - `<flag>`：`wxEXPAND`、`wxALL`、`wxALIGN_CENTER` 等  
  - `<border>`：边距  
  - `<minsize>`、`<ratio>`、`<cellpos>` 等高级属性

布局示例（简化）：

```xml
<object class="wxBoxSizer">
  <orient>wxVERTICAL</orient>

  <object class="sizeritem">
    <object class="wxTextCtrl" name="text"/>
    <option>1</option>
    <flag>wxALL|wxEXPAND</flag>
    <border>10</border>
  </object>

  <object class="sizeritem">
    <object class="wxButton" name="wxID_OK">
      <label>OK</label>
    </object>
    <flag>wxALL|wxALIGN_CENTER</flag>
    <border>10</border>
  </object>
</object>
```

### 2.4 对象复用：`<object_ref>`

`xrc_format.h` 支持在任何可以使用 `<object>` 的地方改用 `<object_ref>`：

```xml
<object class="wxDialog" name="my_dlg">
  ...
</object>

<object_ref name="my_dlg_alias" ref="my_dlg"/>
```

- `ref` 属性指定要引用的已有 `<object>` 的 `name`。  
- 解析时会把被引用的对象复制一份放在 `<object_ref>` 的位置。  
- `<object_ref>` 内写的属性/子节点可以覆盖或追加到原对象上，实现模板化与局部修改。

---

## 3. XML 中的资源引用：图片与字体

### 3.1 `<bitmap>`：图片资源

XRC 中所有图片（工具栏按钮、静态位图、菜单图标等）都通过“位图属性”描述，底层由 `wxXmlResourceHandlerImpl::GetBitmap()` / `GetBitmapBundle()` 解析（`src/xrc/xmlres.cpp`）。

基本形式（`docs/doxygen/overviews/xrc_format.h`）：

```xml
<object class="tool" name="wxID_NEW">
  <tooltip>New</tooltip>
  <bitmap>images/new.png</bitmap>
</object>
```

要点：

- `<bitmap>` 的文本值是一个相对 URL，对应图片文件路径  
- 默认相对于包含该 `<bitmap>` 的 XRC 文件所在目录  
- 注意它是 URL，不是裸文件名，诸如 `#` 等特殊字符需要 URL 转义  
  - 例如路径 `images/#1/tool.png` 的写法为：  
    `<bitmap>images/%231/tool.png</bitmap>`

#### 3.1.1 多分辨率位图 / DPI 适配

支持通过分号分隔的一组位图来描述多分辨率资源（高 DPI）：

```xml
<bitmap>new.png;new_2x.png</bitmap>
```

- 第一张图的大小代表逻辑尺寸，用于 100% DPI  
- 后续图片用于更高 DPI，库根据尺寸与当前缩放比自动选择最合适的位图  
- 示例可见 `samples/xrc/rc/controls.xrc` 中的 toolbar 图标：

```xml
<bitmap>basicdlg.xpm;basicdlg2.xpm</bitmap>
```

底层实现位于 `GetBitmapBundle()`，会解析 `;` 并构造 `wxBitmapBundle`。

#### 3.1.2 使用 wxArtProvider：`stock_id` / `stock_client`

可以不直接指定文件，而是让 `wxArtProvider` 按 ID 提供位图（示例：`samples/xrc/rc/artprov.xrc`）：

```xml
<object class="wxStaticBitmap" name="my_bitmap">
  <bitmap stock_id="wxART_INFORMATION"
          stock_client="wxART_MESSAGE_BOX">
    derivdlg.xpm   <!-- 可选：wxArtProvider 找不到时的回退文件 -->
  </bitmap>
</object>
```

- `stock_id`：必填，字符串形式的 `wxART_*` ID（在代码中通过 `wxART_MAKE_ART_ID_FROM_STR` 转换）。  
- `stock_client`：可选，指定在哪个“客户端”（如 `wxART_TOOLBAR`、`wxART_MENU`）下查找位图；不填则用调用方的默认 client。  
- 如果 `wxArtProvider::GetBitmap()` 找不到资源，则退回到 `<bitmap>` 文本中指定的文件路径。

相关实现：

- 解析 `stock_id` / `stock_client`：`GetStockArtAttrs()`（`src/xrc/xmlres.cpp`）  
- 文件加载：`LoadBitmapFromFS()` 使用 `wxFileSystem` 打开文件，可从普通文件、压缩包、内存 FS 中读取。

#### 3.1.3 SVG 图片

XRC 也支持通过 SVG 文件生成 `wxBitmapBundle`（前提是构建启用 SVG 支持）：

```xml
<bitmap default_size="24,24">icons/app.svg</bitmap>
```

- `default_size`：必填，告知 SVG 逻辑尺寸  
- 解析逻辑同样在 `GetBitmapBundle()` 中，如果是 `.svg` 会走专门分支

### 3.2 `<font>`：字体资源

字体属性在 XRC 中是一个“复合属性”（composite property），使用 `<font>` 节点，子节点描述各个维度（详见 `xrc_format.h` 与 `src/xrc/xmlres.cpp` 中 `wxXmlResourceHandlerImpl::GetFont()`）。

基本结构：

```xml
<font>
  <size>12</size>
  <style>italic</style>
  <weight>bold</weight>
  <family>swiss</family>
  <underlined>1</underlined>
  <face>arial,helvetica</face>
  <encoding>utf-8</encoding>
</font>
```

XRC 支持的子属性（均为可选）：

- `size`（float）：字号（像素/点，依据平台）；  
- `style`：`normal` / `italic` / `slant`；  
- `weight`：可以是字符串（`thin` / `extralight` / `light` / `normal` / `medium` / `semibold` / `bold` / `extrabold` / `heavy` / `extraheavy`）或 1–1000 的整数；  
- `family`：`default` / `roman` / `script` / `decorative` / `swiss` / `modern` / `teletype`；  
- `underlined`：布尔；  
- `strikethrough`：布尔，删除线；  
- `face`：逗号分隔的一组字体名，依次尝试；  
- `encoding`：编码（非 Unicode 构建时有意义）；  
- `sysfont`：系统字体名，如 `wxSYS_DEFAULT_GUI_FONT` 等；  
- `inherit`：布尔，表示从父窗口继承字体；  
- `relativesize`：相对父字体或系统字体的大小缩放因子。

典型例子（出自 `xrc_format.h`）：

```xml
<font>
  <!-- 固定字体：若有 Arial 就用 Arial，否则退回 Helvetica -->
  <face>arial,helvetica</face>
  <size>12</size>
</font>

<font>
  <!-- 基于系统默认 GUI 字体放大并加粗 -->
  <sysfont>wxSYS_DEFAULT_GUI_FONT</sysfont>
  <weight>bold</weight>
  <relativesize>1.5</relativesize>
</font>
```

在控件上的用法（节选自 `samples/xrc/rc/controls.xrc`）：

```xml
<object class="wxStaticText" name="controls_statictext">
  <label>It was a dark and stormy night.</label>
  <font>
    <inherit>1</inherit>
    <weight>700</weight>       <!-- 同 bold -->
  </font>
</object>
```

解析流程（`GetFont()`）：

1. 找到 `<font>` 节点并临时切换当前解析节点  
2. 读取子属性并转换为 `wxFontStyle`、`wxFontWeight`、`wxFontFamily` 等枚举值  
3. 若存在 `sysfont` 或 `inherit`，先以系统字体或父控件字体为基准，再应用其它属性（`relativesize`、`style`、`weight` 等）  
4. 若没有 `sysfont` 与 `inherit`，则直接根据各属性构造一个新的 `wxFont`

控件在创建后会调用 `SetupWindow()`，其中如果存在 `<font>` / `<ownfont>` 属性会自动应用到控件上。

---

## 4. UI 资源的打包与加载方式

XRC 资源在工程中的组织与打包方式主要有三种：

1. 直接加载若干 `.xrc` 与图片文件  
2. 打包为压缩资源文件（`.xrs` 或 `.zip`）  
3. 使用 `wxrc` 把资源编译进 C++ 源码（嵌入资源）

下面按方式说明。

### 4.1 直接使用 .xrc 文件

最简单的方式：在工程目录中保留 `.xrc` 文件及其引用的图片，然后在程序启动时调用：

```cpp
wxXmlResource::Get()->InitAllHandlers();
wxXmlResource::Get()->Load("resource.xrc");   // 单个文件
wxXmlResource::Get()->LoadAllFiles("rc");     // rc 目录下所有 .xrc
```

随后在代码中按名称加载对象：

```cpp
wxDialog dlg;
wxXmlResource::Get()->LoadDialog(&dlg, parent, "my_dialog");
dlg.ShowModal();
```

优点：开发期方便直接修改 XML；缺点：部署时需要携带多个外部文件。

### 4.2 打包为压缩资源：.xrs / .zip

`docs/doxygen/overviews/xrc_format.h` 的 “Packed XRC Files” 部分说明：

- `.zip` 或 `.xrs` 文件本质上都是 ZIP 压缩包  
- 内部可以包含任意多个 `.xrc` 与其依赖的图片等文件  
- 只要工程启用了 `wxFileSystem` 与 `wxArchiveFSHandler`，`wxXmlResource` 就能直接从压缩包中读取资源

使用 `wxrc` 生成 `.xrs`（参考 `docs/doxygen/overviews/xrc.h`）：

```bash
wxrc resource.xrc            # 默认输出 resource.xrs
wxrc resource.xrc -o myres.xrs
```

`.xrs` 本质是 ZIP，只是扩展名不同，可以用任意 ZIP 工具查看/更新。

运行时加载步骤：

```cpp
#include <wx/filesys.h>
#include <wx/fs_arc.h>

bool MyApp::OnInit()
{
    wxFileSystem::AddHandler(new wxArchiveFSHandler); // 让 wxFileSystem 识别 zip/xrs

    wxXmlResource::Get()->InitAllHandlers();
    wxXmlResource::Get()->Load("myres.xrs");
    ...
}
```

`wxXmlResource::Load()` 内部会根据文件扩展名与 URL 形式判断是否为压缩资源，在 `xmlres.cpp` 中通过 `IsArchive()` 和 `#zip:` 语法处理。

### 4.3 嵌入式资源：生成 C++ 源码

有时需要只发布一个可执行文件，不希望额外携带 `.xrc` / `.xrs` / 图片文件。可以用 `wxrc` 把资源转换为 C++ 源文件。

命令示例（来自 `docs/doxygen/overviews/xrc.h`）：

```bash
wxrc resource.xrc -v -c -o resource.cpp
```

常见组合：

- `-c`：生成 C++ 源码，把 XRC 与图片转成二进制数组并注册到 `wxMemoryFSHandler` 中  
- `-e`：连同生成 C++ 头文件，自动声明对应的基类窗口（方便继承）

生成的 `resource.cpp` 中包含一个初始化函数（默认名为 `InitXmlResource`，可通过参数改名），内部调用 `wxMemoryFSHandler` 注册多个内存文件，并让 `wxXmlResource` 从 `memory:` 文件系统加载它们（可从 `utils/wxrc/wxrc.cpp` 中的模板代码看到）。

运行时代码：

```cpp
extern void InitXmlResource();  // 由 wxrc 生成的函数

bool MyApp::OnInit()
{
    wxXmlResource::Get()->InitAllHandlers();
    InitXmlResource();          // 把嵌入的 XRC 注册到内存文件系统
    ...
}
```

优点：部署简单，不依赖外部资源文件；缺点：修改 UI 需要重新编译。

#### 4.3.1 打包整个资源目录

`wxrc` 本身只接受 “XRC 输入文件列表”，不会直接接收目录参数；但它会在解析 XRC 时自动扫描其中的文件引用（如 `<bitmap>`、`<icon>`、`<imagelist>` 等），并把这些图片等一并打包进生成的 C++ 源码里。因此：

- 你只需要把“目录里所有 .xrc 文件”作为输入传给 `wxrc`；  
- `.xrc` 中用相对路径引用到的图片、图标等资源会被自动找到并嵌入；
- 没有在任何 .xrc 里被引用的文件不会被打包。

假设资源布局为：

```text
res/
  main.xrc
  dialogs.xrc
  images/
    icon16.png
    icon32.png
  toolbar/
    new.png
    open.png
```

其中 `.xrc` 内类似这样引用图片：

```xml
<bitmap>images/icon16.png</bitmap>
<bitmap>toolbar/new.png;toolbar/new_2x.png</bitmap>
```

打包整个资源目录为嵌入式 C++：

- 在类 Unix shell（bash/zsh）中：

```bash
wxrc -c -o res_xrc.cpp res/*.xrc
```

- 在 Windows PowerShell 中（两种写法都可）：

```powershell
wxrc -c -o res_xrc.cpp res\*.xrc
# 或
wxrc -c -o res_xrc.cpp (Get-ChildItem res -Filter *.xrc).FullName
```

如需递归子目录中所有 `.xrc`：

```powershell
wxrc -c -o res_xrc.cpp (Get-ChildItem res -Filter *.xrc -Recurse).FullName
```

`wxrc` 会：

1. 依次解析所有传入的 `.xrc`；  
2. 根据 XRC 内的相对路径（相对于每个 `.xrc` 文件所在目录）找到并复制图片等资源；  
3. 将这些文件全部转成 C++ 数组，写入 `res_xrc.cpp`；  
4. 在生成的 `InitXmlResource()` 中：
   - 使用 `wxMemoryFSHandler` 注册所有文件到内存文件系统（路径形如 `memory:XRC_resource/...`）；  
   - 再调用 `wxXmlResource::Get()->Load("memory:XRC_resource/xxx.xrc")` 载入每个 XRC。

因此，只要在程序初始化中调用一次 `InitXmlResource()`，就相当于加载了整个 `res/` 目录下的所有 XRC 与其引用到的图片资源。

### 4.4 从内存 / 自定义来源加载

如果 XRC 数据来自网络、加密文件或其他自定义来源，可以自己构造 `wxXmlDocument` 再交给 `wxXmlResource`：

```cpp
const char* xrc_data = ...;  // 从任意地方得到的 XRC 文本
wxMemoryInputStream mis(xrc_data, strlen(xrc_data));
auto xmlDoc = std::make_unique<wxXmlDocument>(mis, "UTF-8");

if ( xmlDoc->IsOk() )
{
    wxXmlResource::Get()->LoadDocument(xmlDoc.release()); // 接管所有权
}
```

这在需要动态生成或修改 XRC 时非常有用。

---

## 5. 与当前源码中的示例对应

可在当前 wxWidgets 源码中找到上述机制的具体示例：

- UI 布局：
  - `samples/xrc/rc/frame.xrc`、`samples/xrc/rc/controls.xrc` 展示典型的窗口布局、sizer 使用方式  
  - `samples/propgrid/propgrid.xrc` 展示复杂控件（`wxPropertyGridManager`）在 XRC 中的描述方式
- 图片资源：
  - `samples/xrc/rc/toolbar.xrc` / `menu.xrc` / `resource.xrc` 展示 `<bitmap>` 使用相对路径与多分辨率文件  
  - `samples/xrc/rc/artprov.xrc` 演示如何在 XRC 中通过 `stock_id` / `stock_client` 使用 `wxArtProvider`
- 字体资源：
  - `samples/xrc/rc/controls.xrc` 中多处 `<font>` 示例（指定大小、粗体、继承父字体等）
- 打包与加载：
  - XRC 打包说明在 `docs/doxygen/overviews/xrc.h` 与 `docs/doxygen/overviews/xrc_format.h` 的 “Packed XRC Files” 一节  
  - 工具 `utils/wxrc/wxrc.cpp` 展示了 `.xrs` 和嵌入式 C++ 资源的生成逻辑

在实际工程中，可以以这些示例为模板：

1. 用 XRC 描述窗口、控件和 sizer 布局  
2. 在控件上按需添加 `<font>` / `<bitmap>` 等资源属性  
3. 选择适合的打包方式（直接 .xrc、压缩包 .xrs、或嵌入式 C++ 资源）纳入构建流程  
4. 在应用初始化阶段调用 `wxXmlResource::Get()` 相关接口完成资源加载

这样即可在当前 wxWidgets 项目中形成一套完整、可维护的 XML UI 与资源体系。
