# wxWidgets XML（XRC）UI 编程

> 基于当前仓库中的 `samples/xrc` 示例和 `src/xrc/xmlres.*`, `include/wx/xrc/xmlres.h` 等代码整理。

## 1. 概览：什么是 XRC

- XRC（XML Resource）是 wxWidgets 的 XML‑based 资源系统，用来描述对话框、窗口、菜单栏、工具栏等 UI。
- XRC 文件在运行时由 `wxXmlResource` 解析并实例化为真正的 `wxWindow`/`wxMenuBar` 等对象。
- 典型优势：
  - UI 修改不需要重新编译 C++ 代码，只要替换 XRC 文件即可。
  - UI 结构与业务逻辑解耦，便于多人协作和自动化生成（UI 设计器）。
  - 全面使用 sizer 布局，天然跨平台、自适应大小。

XRC 的实现集中在：

- 核心类：`wxXmlResource`（见 `include/wx/xrc/xmlres.h`，`src/xrc/xmlres.cpp`）
- 示例：`samples/xrc` 目录（强烈建议边跑示例边对照阅读）

以下小节以 `samples/xrc` 为主线说明完整工作流。

## 2. XRC 文件结构

XRC 文件本质是普通 XML，根节点通常为 `<resource>`：

```xml
<?xml version="1.0" encoding="ISO-8859-1"?>

<resource xmlns="http://www.wxwidgets.org/wxxrc" version="2.5.3.0">

<object class="wxFrame" name="main_frame">
    <title>XML Resources Demo</title>
    <centered>1</centered>
    <object class="wxFlexGridSizer">
        <cols>1</cols>
        …
    </object>
</object>

</resource>
```

关键概念：

- `<resource>`：根节点，`version` 存储 XRC 版本（对应 `WX_XMLRES_CURRENT_VERSION_*` 宏）。
- `<object>`：
  - `class` 属性对应要创建的 C++ 类型，如 `wxDialog`、`wxFrame`、`wxButton` 等。
  - `name` 属性对应控件的“名字”，用于查找和事件绑定（`XRCID` / `XRCCTRL`）。
- 布局：
  - 常见 `class`：`wxBoxSizer`、`wxFlexGridSizer`、`wxGridSizer` 等。
  - 子项使用 `<object class="sizeritem">` 包裹，里面再嵌套具体控件或 sizer。
  - sizeritem 上常见属性：
    - `<flag>`：布局标志（`wxEXPAND`、`wxALIGN_CENTER` 等）。
    - `<border>`：边距。
    - `<option>`：伸展比例。

简单对话框示例（`samples/xrc/rc/basicdlg.xrc`）：

```xml
<object class="wxDialog" name="non_derived_dialog">
    <title>Non-Derived Dialog Example</title>
    <centered>1</centered>
    <object class="wxBoxSizer">
        <orient>wxVERTICAL</orient>
        <object class="sizeritem">
            <object class="wxTextCtrl" name="message_textctrl">
                <size>500,150</size>
                <style>wxTE_MULTILINE</style>
                <value>...</value>
            </object>
            <flag>wxGROW|wxALL</flag>
            <border>5</border>
        </object>
        <object class="sizeritem">
            <object class="wxStdDialogButtonSizer">
                …
            </object>
            <flag>wxEXPAND|wxALL</flag>
            <border>5</border>
        </object>
    </object>
</object>
```

其他一些特殊 `class`：

- `wxMenuBar`、`wxMenu`、`wxMenuItem`、`tool`、`spacer`、`notebookpage` 等（见 `samples/xrc/rc/resource.xrc`）。
- `unknown`：占位自定义控件（见 `custclas.xrc`）。
- `object_ref`：对象引用（见 `objref.xrc`）。
- `ids-range`：ID 范围定义（见 `objref.xrc` 末尾）。

## 3. 应用启动：加载 XRC 资源

参考 `samples/xrc/xrcdemo.cpp:MyApp::OnInit`：

```cpp
bool MyApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

#if wxUSE_XPM
    wxImage::AddHandler(new wxXPMHandler);
#endif
#if wxUSE_GIF
    wxImage::AddHandler(new wxGIFHandler);
#endif

    wxXmlResource::Get()->InitAllHandlers();

    wxXmlResource::Get()->SetFlags(wxXRC_USE_LOCALE | wxXRC_USE_ENVVARS);

#if wxUSE_RIBBON
    wxXmlResource::Get()->AddHandler(new wxRibbonXmlHandler);
#endif
#if wxUSE_AUI
    wxXmlResource::Get()->AddHandler(new wxAuiXmlHandler);
    wxXmlResource::Get()->AddHandler(new wxAuiToolBarXmlHandler);
#endif

    if ( !wxXmlResource::Get()->LoadAllFiles("rc") )
        return false;

    MyFrame* frame = new MyFrame();
    frame->Show(true);
    return true;
}
```

要点：

- 图片 handler 需要提前注册，保证 XRC 中引用的 `.xpm`/`.gif` 能被正确加载。
- `wxXmlResource::Get()` 获取全局单例：
  - `InitAllHandlers()` 注册所有内置 XRC 控件 handler。
  - 如需仅使用少数控件，可以不调用 `InitAllHandlers()`，改为按需 `AddHandler(new ...)` 节省体积。
- `SetFlags()` 配置行为：
  - `wxXRC_USE_LOCALE`：使用 `_()` 对可翻译字符串做本地化。
  - `wxXRC_USE_ENVVARS`：对路径类属性调用 `wxExpandEnvVars`（见 `src/xrc/xmlres.cpp` 中 `wxXRC_USE_ENVVARS` 判断）。
- 加载资源：
  - `Load("foo.xrc")`：加载单个文件，支持通配符。
  - `LoadAllFiles("rc")`：加载目录下所有 `.xrc` 文件（示例项目采用）。
  - 一般在应用启动时一次性加载即可。

## 4. 从 XRC 创建顶层窗口

### 4.1 Frame

`MyFrame` 的构造函数中（见 `samples/xrc/myframe.cpp`）：

```cpp
MyFrame::MyFrame(wxWindow* parent)
       : wxFrame()
{
    wxXmlResource::Get()->LoadFrame(this, parent, "main_frame");

    SetMenuBar(wxXmlResource::Get()->LoadMenuBar("main_menu"));
    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, "main_toolbar"));

#if wxUSE_STATUSBAR
    CreateStatusBar(1);
#endif

    GetSizer()->SetSizeHints(this);
}
```

注意：

- `LoadFrame(this, parent, "main_frame")` 会以 XRC 中的 `wxFrame` 定义填充当前对象。
- `LoadMenuBar`/`LoadToolBar` 根据给定名称从 XRC 创建菜单栏和工具栏。
- 加载完 frame 后，如果又动态添加状态栏/工具栏，记得让 sizer 重新计算最小尺寸（示例里通过 `GetSizer()->SetSizeHints(this)` 完成）。

### 4.2 非派生对话框（“临时弹窗”型）

典型代码（`MyFrame::OnNonDerivedDialogToolOrMenuCommand`）：

```cpp
void MyFrame::OnNonDerivedDialogToolOrMenuCommand(wxCommandEvent&)
{
    wxDialog dlg;
    if ( wxXmlResource::Get()->LoadDialog(&dlg, this, "non_derived_dialog") )
        dlg.ShowModal();
}
```

特点：

- `wxDialog` 直接实例化，没有自定义子类。
- 适合 About / 简单提示类对话框：只有内置 OK/Cancel 行为，无需复杂事件处理。

### 4.3 派生对话框（推荐方式）

以 `PreferencesDialog`（见 `samples/xrc/derivdlg.*`）为例：

```cpp
// derivdlg.h 中声明:
class PreferencesDialog : public wxDialog
{
public:
    PreferencesDialog(wxWindow* parent);
    …
};

// derivdlg.cpp 中构造函数:
PreferencesDialog::PreferencesDialog(wxWindow* parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "derived_dialog");
}
```

调用处（`MyFrame::OnDerivedDialogToolOrMenuCommand`）：

```cpp
void MyFrame::OnDerivedDialogToolOrMenuCommand(wxCommandEvent&)
{
    PreferencesDialog dlg(this);
    dlg.ShowModal();
}
```

特点：

- UI 布局完全在 XRC 中描述；构造函数只负责把 XRC “套”到派生类实例上。
- 事件处理全部写在 `PreferencesDialog` 类里，便于封装和复用。

## 5. 控件查找与 ID 映射

### 5.1 XRCID：从 name 到 int ID

定义位置：`include/wx/xrc/xmlres.h:468` 左右：

```cpp
#define XRCID(str_id) \
    wxXmlResource::DoGetXRCID(str_id)
```

说明：

- 传入的是 XRC 中控件的 `name` 字符串，例如 `"my_button"`、`"digits[3]"`。
- 返回值是一个唯一的整数 ID，可用于：
  - 静态事件表（`EVT_MENU(XRCID("..."), ...)`）。
  - `Bind`/`Connect` 的 ID 区间。
- XRC 会处理与 `wxID_OK` 等标准 ID 的映射，`XRCID("wxID_OK")` 等价于 `wxID_OK`。

示例：`samples/xrc/myframe.cpp` 的菜单事件表：

```cpp
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(XRCID("unload_resource_menuitem"), MyFrame::OnUnloadResourceMenuCommand)
    EVT_MENU(XRCID("reload_resource_menuitem"), MyFrame::OnReloadResourceMenuCommand)
    EVT_MENU(wxID_EXIT, MyFrame::OnExitToolOrMenuCommand)
    …
wxEND_EVENT_TABLE()
```

### 5.2 XRCCTRL：从容器 + name 获取控件指针

宏定义（`include/wx/xrc/xmlres.h:474`）：

```cpp
#define XRCCTRL(window, id, type) \
    (wxStaticCast((window).FindWindow(XRCID(id)), type))
```

用法：在已经通过 XRC 创建好的窗口中，按 `name` 获取子控件。

派生对话框中访问 checkbox / textctrl（`derivdlg.cpp`）：

```cpp
bool checked = XRCCTRL(*this, "my_checkbox", wxCheckBox)->IsChecked();
XRCCTRL(*this, "my_textctrl", wxTextCtrl)->Enable(checked);
```

列表控件初始化（`myframe.cpp::OnControlsToolOrMenuCommand`）：

```cpp
wxListCtrl* list = XRCCTRL(dlg, "controls_listctrl", wxListCtrl);
list->InsertItem(0, "Athos", 0);
…
```

注意事项：

- 第一个参数是父窗口或对话框，通常是 `*this` 或局部的 `dlg`。
- 不要与控件的 `label` 混淆，XRCID/XRCCTRL 使用的是 `name` 属性。

### 5.3 XRCSIZERITEM：获取 sizer item

宏定义（`include/wx/xrc/xmlres.h`）：

```cpp
#define XRCSIZERITEM(window, id) \
    ((window).GetSizer() ? (window).GetSizer()->GetItemById(XRCID(id)) : nullptr)
```

用途：根据 XRC 中为 `sizeritem` 配置的 `name` 获取对应 `wxSizerItem*`，可以在运行时修改间距、隐藏子项等。示例可见 `docs/doxygen/overviews/xrc.h`。

## 6. 自定义控件与扩展

wxWidgets 提供三种主要方式把自定义 C++ 控件与 XRC 集成：

1. 使用 `subclass` 属性（简单场景）。
2. 使用 `<object class="unknown">` + `AttachUnknownControl()`（`samples/xrc` 的主要示例）。
3. 编写自定义 `wxXmlResourceHandler`（复杂场景，参见 `docs/doxygen/overviews/xrc.h`）。

### 6.1 unknown + AttachUnknownControl（示例：自适应 ListCtrl）

XRC 占位符（`samples/xrc/rc/custclas.xrc`）：

```xml
<object class="wxDialog" name="custom_class_dialog">
    …
    <object class="sizeritem">
        <object class="unknown" name="custom_control_placeholder">
            <size>100,100</size>
        </object>
    </object>
    …
</object>
```

自定义控件类（`samples/xrc/custclas.cpp`）：

```cpp
wxIMPLEMENT_DYNAMIC_CLASS(MyResizableListCtrl, wxListCtrl);

wxBEGIN_EVENT_TABLE(MyResizableListCtrl, wxListCtrl)
    EVT_RIGHT_DOWN(MyResizableListCtrl::ContextSensitiveMenu)
    EVT_SIZE(MyResizableListCtrl::OnSize)
wxEND_EVENT_TABLE()

MyResizableListCtrl::MyResizableListCtrl(wxWindow* parent, wxWindowID id,
                                         const wxPoint& pos, const wxSize& size,
                                         long style, const wxValidator& validator,
                                         const wxString& name)
    : wxListCtrl(parent, id, pos, size, style, validator, name)
{
    InsertColumn(0, _("Record"),   wxLIST_FORMAT_LEFT, 140);
    InsertColumn(1, _("Action"),   wxLIST_FORMAT_LEFT, 70);
    InsertColumn(2, _("Priority"), wxLIST_FORMAT_LEFT, 70);
}
```

在对话框打开前将其塞进占位符（`myframe.cpp::OnCustomClassToolOrMenuCommand`）：

```cpp
void MyFrame::OnCustomClassToolOrMenuCommand(wxCommandEvent&)
{
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, "custom_class_dialog");

    MyResizableListCtrl* ctrl = new MyResizableListCtrl(
        &dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT | wxLC_SINGLE_SEL, wxDefaultValidator);

    wxXmlResource::Get()->AttachUnknownControl(
        "custom_control_placeholder", ctrl, &dlg);

    dlg.ShowModal();
}
```

步骤总结：

1. 在 XRC 中用 `<object class="unknown" name="占位名">` 定义占位控件。
2. 在 C++ 中创建自定义控件实例。
3. 调用 `AttachUnknownControl("占位名", 指针, 父窗口)` 将其挂接到占位位置。

### 6.2 subclass 属性（在 XML 中直接使用派生控件）

虽然当前 `samples/xrc` 没有直接使用 `subclass`，但官方文档（`docs/doxygen/overviews/xrc.h`、`xrc_format.h`）推荐它作为“轻量级自定义控件”的首选方式。

**基本写法**

- XML：

```xml
<object class="wxTextCtrl" name="text" subclass="MyTextCtrl">
    <style>wxTE_MULTILINE</style>
    <!-- 其他 wxTextCtrl 支持的属性照常写 -->
</object>
```

- C++：

```cpp
class MyTextCtrl : public wxTextCtrl
{
public:
    MyTextCtrl() = default;              // 必须有默认构造函数

    // 可选：覆写事件 / 绘制等
    // void OnSomething(...);

    wxDECLARE_DYNAMIC_CLASS(MyTextCtrl);
};

wxIMPLEMENT_DYNAMIC_CLASS(MyTextCtrl, wxTextCtrl);
```

此时 XRC 在加载 `<object class="wxTextCtrl" subclass="MyTextCtrl">` 时：

- 实际构造的是 `MyTextCtrl`；
- 但后续会像普通 `wxTextCtrl` 一样调用 `Create()` 并设置属性、加载子对象。

**subclass 方式的约束（来自 `xrc_format.h`）**

使用 `subclass` 时，自定义类必须满足：

- 必须从 `class` 指定的类型派生（例子里是 `wxTextCtrl`）。
- 必须支持 wxWidgets 的 RTTI：有 `wxDECLARE_DYNAMIC_CLASS` / `wxIMPLEMENT_DYNAMIC_CLASS`。
- 必须支持两阶段创建（two‑phase create）：
  - 需要默认构造函数；
  - XRC 会调用基类的 `Create()`，不能依赖自定义的 `Create()` 签名。
- 不能改变控件的“创建方式”，而应该通过重载虚函数 / 事件处理来添加行为。

这意味着：

- `subclass` 适合“在现有控件基础上加一点行为”的场景（重绘、事件处理、内部状态），XML 里的属性和结构仍按原控件写。
- 不适合需要额外 XML 属性、复杂子结构、特殊构造参数的自定义控件，此时应改用 `unknown` + `AttachUnknownControl` 或完全自定义 handler。

**在代码中访问 subclass 控件**

XRC 会把该控件视为 `MyTextCtrl`，因此可以直接：

```cpp
MyTextCtrl* txt = XRCCTRL(dlg, "text", MyTextCtrl);
```

### 6.3 自定义 wxXmlResourceHandler：在 XML 中直接使用自定义 class

如果希望在 XML 里像使用内置控件一样直接写自定义控件类名（并能定义自己的 XML 属性），需要编写 `wxXmlResourceHandler`：

```xml
<object name="my_ctrl" class="MyWidget">
    <my_prop>foo</my_prop>
    <!-- 这里的子节点由自定义 handler 解析 -->
</object>
```

大致步骤（参见 `docs/doxygen/overviews/xrc_format.h` 的 “Adding Custom Classes” 小节）：

1. 定义控件类：
   - 从 `wxWindow` / `wxControl` 等派生，并最终从 `wxObject` 派生；
   - 实现 RTTI：`wxDECLARE_DYNAMIC_CLASS` / `wxIMPLEMENT_DYNAMIC_CLASS`。
2. 定义 handler：

```cpp
class MyWidgetXmlHandler : public wxXmlResourceHandler
{
    wxDECLARE_DYNAMIC_CLASS(MyWidgetXmlHandler);

public:
    MyWidgetXmlHandler() { AddWindowStyles(); }

    bool CanHandle(wxXmlNode* node) override
    {
        return IsOfClass(node, "MyWidget");
    }

    wxObject* DoCreateResource() override
    {
        MyWidget* w = new MyWidget(m_parentAsWindow);
        // 从 XML 读自定义属性，例如:
        // wxString v = GetText("my_prop", "default");
        // w->SetMyProp(v);
        SetupWindow(w);
        return w;
    }
};
```

3. 在应用初始化中注册 handler：

```cpp
wxXmlResource::Get()->AddHandler(new MyWidgetXmlHandler);
```

之后就可以在任意 XRC 文件中直接写：

```xml
<object class="MyWidget" name="my_ctrl">
    <my_prop>foo</my_prop>
</object>
```

与 `subclass` 相比：

- `subclass`：XML 属性完全沿用基类控件，只是换了 C++ 类型；
- 自定义 handler：可以定义全新的 XML 语法和属性，XRC 解析逻辑由你控制，适合封装复杂组件或顶层窗口。

## 7. 对象复用与 ID 范围（objref.xrc）

### 7.1 object_ref：在多个位置复用一段 UI

在 `samples/xrc/rc/objref.xrc` 中，底部公共区域被单独定义为一个 `wxPanel`：

```xml
<object class="wxPanel" name="bottom_panel">
    …
</object>
```

然后在 notebook 的多个页面中通过 `<object_ref>` 复用：

```xml
<object class="notebookpage">
  <object class="wxPanel" name="page1">
    <object class="wxFlexGridSizer">
      …
      <object class="sizeritem">
        <object_ref ref="bottom_panel"/>
        <flag>wxEXPAND</flag>
      </object>
    </object>
  </object>
  <label>Page 1</label>
</object>
```

也可以整个页面复用：

```xml
<object class="notebookpage">
  <object_ref ref="page1"/>
  <label>Page 1 copy</label>
</object>
```

运行时，`wxXmlResource` 会在解析 `<object_ref>` 时将引用节点展开为真实控件树（见 `src/xrc/xmlres.cpp` 中的 `CreateResFromNode()` 和相关逻辑）。

### 7.2 ids-range：管理一组相似控件

`objref.xrc` 文件尾部定义了若干 ID 范围：

```xml
<ids-range name="check" size="3" start="10000"/>
<ids-range name="first_row"/>
<ids-range name="second_row"/>
<ids-range name="third_row"/>
<ids-range name="digits" size="8"/>
<ids-range name="operators"/>
```

配套控件命名例如：

```xml
<object class="wxStaticBitmap" name="first_row[0]"> … </object>
<object class="wxStaticBitmap" name="first_row[1]"> … </object>
<object class="wxStaticBitmap" name="first_row[2]"> … </object>
```

XRC 加载时，由 `wxIdRangeManager`（见 `src/xrc/xmlres.cpp`）负责：

- 为同一 range 内元素分配连续的 ID；
- 自动提供两个特别的“虚拟元素”：
  - `<range>[start]`：指向首元素；
  - `<range>[end]`：指向尾元素（即使没有显式对象使用这个名字）。

在代码中可以方便地针对一整段 ID 做绑定或判断，例如（节选自 `samples/xrc/objrefdlg.cpp`）：

```cpp
wxNotebookPage* page = nb->GetPage(icons_page);
page->Bind(wxEVT_UPDATE_UI, &ObjrefDialog::OnUpdateUIFirst, this,
           XRCID("first_row[start]"), XRCID("first_row[end]"));
```

计算按钮索引：

```cpp
void ObjrefDialog::OnNumeralClick(wxCommandEvent& event)
{
    int digit = event.GetId() - XRCID("digits[start]");
    …
}
```

使用 ID 范围的好处：

- XRC 文件中命名统一：`digits[0]` ~ `digits[9]`；
- C++ 代码中可以用 ID 区间或偏移实现简洁循环。

## 8. 递归加载单个控件：LoadObjectRecursively

有时只想从某个复杂 XRC 对话框中拿出其中一个子控件，插入到自己手写的窗口中。`wxXmlResource::LoadObjectRecursively()` 用于这种场景。

示例（`samples/xrc/myframe.cpp::OnRecursiveLoad`）：

```cpp
void MyFrame::OnRecursiveLoad(wxCommandEvent&)
{
    wxDialog dlg(nullptr, wxID_ANY, "Recursive Load Example",
                 wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(&dlg, wxID_ANY,
                                "The entire tree book control below is loaded from XRC"),
               wxSizerFlags().Expand().Border());

    sizer->Add(
        static_cast<wxWindow*>(
            wxXmlResource::Get()->
                LoadObjectRecursively(&dlg, "controls_treebook", "wxTreebook")
        ),
        wxSizerFlags(1).Expand().Border()
    );

    dlg.SetSizer(sizer);
    dlg.SetClientSize(400, 200);
    dlg.ShowModal();
}
```

说明：

- `"controls_treebook"` 是定义在 `controls.xrc` 里的某个子控件名字；
- 该控件嵌套较深，`LoadObject()` 找不到，而 `LoadObjectRecursively()` 会在整个已加载的资源树中按广度优先搜索；
- 成功后返回的对象已经是一个完全初始化好的 `wxTreebook`，可直接加入当前 sizer。

## 9. 变量与环境变量展开

### 9.1 环境变量展开

`wxXmlResource` 支持对某些路径类属性执行环境变量展开。例如在 XRC 中写：

```xml
<bitmap>$(MY_APP_DATA)/images/icon.png</bitmap>
```

前提：

- 在代码中启用 `wxXRC_USE_ENVVARS` 标志（见 `xrcdemo.cpp` 中的 `SetFlags(wxXRC_USE_LOCALE | wxXRC_USE_ENVVARS)`）。
- 运行时环境中设置好对应的环境变量。

内部实现位于 `src/xrc/xmlres.cpp` 中 `wxXRC_USE_ENVVARS` 分支，对路径调用 `wxExpandEnvVars()`。

### 9.2 $(var) 形式的普通变量

`variable.xrc` 中演示了在文本里使用 `$(version)` 形式的变量：

```xml
<object class="wxStaticText" name="version_statictext">
    <label>$(version)</label>
</object>
```

XRC 头部注释中提到需要调用类似 `SetVariable("version", "2.4.0")` 的 API 进行赋值，但当前仓库代码中并未实现该接口，对应 XRC 里的说明文本也写着 “VARIABLE EXPANSION ISN'T IMPLEMENTED CURRENTLY”。

结论：

- 环境变量展开（`wxXRC_USE_ENVVARS`）已实现并在 `xrcdemo` 中使用；
- 文本中任意变量替换（`$(var)`）属于历史/计划特性，目前以文档和示例说明为主，实际支持情况需以所用 wxWidgets 版本的实现和官方手册为准。

## 10. 二进制/打包资源（简要）

`docs/doxygen/overviews/xrc.h` 还介绍了 XRC 的两种打包形式：

- `.xrs`：通过 `wxrc` 工具将多个 XRC 编译成单个二进制资源文件；
- 生成 C++ 数组：可把资源编译进可执行文件中，便于部署。

当前 `samples/xrc` 直接从磁盘加载 `.xrc`，便于阅读和调试；在实际项目中可以在开发阶段使用 plain XRC，发布时再切换到 `.xrs` 或内嵌 C++ 资源以减少文件数量。

## 11. 项目中使用 XRC 的推荐流程

结合仓库中的实践，可以按照以下步骤在自己的 wxWidgets 应用中引入 XML‑based UI：

1. **规划资源结构**
   - 建议单独建一个 `rc` 目录存放所有 `.xrc` 和图片资源；
   - 按功能拆分多个 XRC 文件（frame、菜单、各类对话框）。

2. **在 App 启动时加载资源**
   - 在 `MyApp::OnInit()` 中：
     - 注册需要的 `wxImage` handler；
     - 调用 `wxXmlResource::Get()->InitAllHandlers()`；
     - 设置 `SetFlags(...)`（至少 `wxXRC_USE_LOCALE`，按需启用 `wxXRC_USE_ENVVARS`）；
     - 调用 `Load()` / `LoadAllFiles()` 加载所有 `.xrc`。

3. **为主窗口和主菜单建 XRC**
   - 用 `<object class="wxFrame" name="main_frame">` 描述主窗口；
   - 用单独的 XRC 文件描述菜单栏、工具栏；
   - 在主 frame 构造函数中调用 `LoadFrame` + `LoadMenuBar` + `LoadToolBar`。

4. **所有复杂对话框都派生自 wxDialog**
   - 每个对话框对应一个 `MyDialog : public wxDialog`；
   - 构造函数中调用 `LoadDialog(this, parent, "dialog_name")`；
   - 使用 `XRCCTRL` 获取子控件指针；
   - 在事件表或 `Bind` 中用 `XRCID("控件名")` 绑定事件。

5. **需要复用 UI 时使用 object_ref / ids-range**
   - 复杂布局中的公用区域拆成单独 `<object>`，在其他位置用 `<object_ref ref="..."/>` 引用；
   - 对结构相似的一组控件使用 `ids-range` + 下标命名，例如 `digits[0]`…`digits[9]`。

6. **自定义控件优先考虑 subclass，其次 unknown**
   - 简单“换类”用 `subclass="MyControl"`；
   - 无法纯粹在 XML 中描述的复杂控件，使用 `<object class="unknown">` + `AttachUnknownControl`。

---

以上内容结合了当前仓库中 XRC 的实现与示例代码，涵盖了基于 XML 的 wxWidgets UI 编程的主要模式和进阶特性。实际项目中可以直接按 `samples/xrc` 的结构起步，再逐步引入 object_ref、ids-range、自定义控件等高级用法。
