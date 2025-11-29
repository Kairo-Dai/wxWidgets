# wxWidgets UI 布局：Sizer、窗口尺寸与实践技巧

> 本文基于当前仓库中的实现与示例整理，包括：
>
> - Sizer 布局系统的核心思想与几种常用 sizer 类型  
> - 顶层窗口、对话框和组合控件内部的典型布局模式  
> - 源码中常见的布局技巧  
> - sizer 相关的典型坑位与调试手段
>
> 主要参考代码（可在当前仓库中查看）：
>
> - Sizer 总览文档：`docs/doxygen/overviews/sizer.h`:10  
> - 窗口尺寸与 sizer 关系：`docs/doxygen/overviews/windowsizing.h`:10  
> - Sizer 实现与调试断言：`src/common/sizer.cpp`:200  
> - 典型布局示例：`samples/layout/layout.cpp`:117、407、530  
> - 对话框布局示例：`src/generic/aboutdlgg.cpp`:120、260  
> - 项目示例：`build-my/WxwidgetPro/BorderlessRoundedWindowBase/ExampleBorderlessRoundedWindow.cpp`:33

---

## 1. 总览：wxWidgets 的布局体系

- **布局核心是 Sizer**  
  所有现代 wxWidgets 界面基本都依赖 `wxSizer` 及其子类完成布局。  
  Sizer 根据每个子控件的 *best size* / *min size* 自动分配空间，避免平台差异。

- **窗口尺寸相关概念**（见 `windowsizing.h`:10）：  
  - *Best Size*：控件根据内容计算出的理想大小（按钮按文本，列表按项数等）。  
  - *Minimal Size*：开发者通过 `SetMinSize()`/`SetSizeHints()` 或构造中的 size 指定的下限。  
  - *Initial Size*：构造时传入的尺寸；多数控件会同时用它作为最小尺寸。  
  - Sizer 通过 `wxWindow::GetEffectiveMinSize()` 综合 best/min size 决定布局需求。

- **主要布局方式**
  - Sizer 布局：`wxBoxSizer`、`wxFlexGridSizer`、`wxGridBagSizer`、`wxWrapSizer` 等。  
  - XRC 中的 sizer 布局（见 `UI与资源.md`，配合 `xrc.h`）。  
  - 绝对布局（直接 `SetSize` / `Move`），仅在少量特效窗口中使用。  
  - 高层布局管理器：`wxAuiManager`（`aui.h`:10）用 sizer 实现可拖拽停靠。

实践上，**除非极其特殊的效果窗口，UI 应全部由 sizer 驱动**。

---

## 2. Sizer 的工作方式与通用参数

### 2.1 Sizer 的层级结构

- 每个 sizer 负责 *一个容器窗口* 的子元素布局：  
  - 容器常见的是 `wxPanel`、`wxDialog`、`wxFrame` 的 client 区。  
  - 也可以是组合控件内部的面板，例如 `wxGenericAboutDialog` 将内容放在 `m_contents` 面板上：`src/generic/aboutdlgg.cpp`:120。
- Sizer 本身不是 `wxWindow`（见 `sizer.h`:32），不会影响 tab 顺序，也非常轻量。  
- Sizer 可以嵌套：一个 sizer 可以 Add 另一个 sizer，从而构成树形布局。

典型写法（参考 `samples/layout/layout.cpp`:117）：

```cpp
wxPanel* p = new wxPanel(this, wxID_ANY);

wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
// ... 往 topsizer Add 各种控件或子 sizer ...

p->SetSizer(topsizer);
topsizer->SetSizeHints(this); // 约束顶层窗口最小尺寸
```

### 2.2 Add 的三个核心参数

无论是直接调用 `Add`，还是通过 `wxSizerFlags` 链式调用，本质都会落到：

```cpp
Add(window_or_sizer, proportion, flags, border);
```

含义（见 `sizer.h`:180 之后的例子）：

- `proportion`：在 **主布局方向** 上如何分配剩余空间。  
  - 同一个 sizer 内所有子项的 proportion 之和 = 总权重。  
  - 0 表示不参与分配，只保持最小尺寸。  
  - >0 表示可伸缩；常见写法是顶部内容 `1`，底部按钮 `0`。

- `flags`：一组按位或的布局标志，比如：
  - 对齐：`wxALIGN_LEFT / RIGHT / CENTER_HORIZONTAL / TOP / BOTTOM / CENTER_VERTICAL` 等；  
  - 填充：`wxEXPAND`（在次方向上拉伸填满）；  
  - 边距方位：`wxALL / wxLEFT / wxRIGHT / wxTOP / wxBOTTOM`；  
  - 特例：`wxSHAPED`、`wxRESERVE_SPACE_EVEN_IF_HIDDEN` 等。

- `border`：指定边距厚度（像素或 DIP，内部会换算，见 `wxSizerFlags::GetDefaultBorderInPx()`：`src/common/sizer.cpp`:120）。

推荐写法是使用 `wxSizerFlags`（见 `sizer.h`:270、273）：

```cpp
topsizer->Add(
    new wxTextCtrl(p, wxID_ANY, "..."),
    wxSizerFlags(1).Expand().Border(wxALL)
);
```

优点是可读性更高、参数顺序清晰。

---

## 3. 常用 Sizer 类型与使用场景

### 3.1 wxBoxSizer / wxStaticBoxSizer：最常用的一维布局

- 文档：`docs/doxygen/overviews/sizer.h`:136  
- 特点：
  - 沿一个方向（水平/垂直）顺序排列子项；  
  - 通过 `proportion + wxEXPAND` 控制各项在主方向上的伸缩；  
  - 通过 `wxALIGN_xxx` 控制在次方向上的对齐。

示例：主窗口顶部文本，中间可伸缩编辑框，底部按钮（`samples/layout/layout.cpp`:117）：

```cpp
wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

topsizer->Add(
    new wxStaticText(p, wxID_ANY, "An explanation ..."),
    wxSizerFlags().Align(wxALIGN_LEFT).Border(wxALL & ~wxBOTTOM)
);

topsizer->Add(
    new wxTextCtrl(p, wxID_ANY, "My text.", wxDefaultPosition,
                   FromDIP(wxSize(100, 60), p), wxTE_MULTILINE),
    wxSizerFlags(1).Expand().Border(wxALL)
);

wxBoxSizer* button_box = new wxBoxSizer(wxHORIZONTAL);
button_box->Add(new wxButton(p, wxID_ANY, "OK"),
                wxSizerFlags().Border(wxALL, FromDIP(7, p)));

topsizer->Add(button_box, wxSizerFlags().Center());
```

`wxStaticBoxSizer` 只是加了一层 `wxStaticBox` 边框（同样在 `layout.cpp`:137 一段有示例），常见于把一组相关控件包起来。

布局技巧：

- 多用嵌套：大框用垂直 box sizer，小区域再用水平 box sizer 细分。  
- 边距统一使用 `wxSizerFlags().Border()` / `.DoubleBorder()`，保证视觉一致。  
- 对话框中常见模式是顶部内容 `proportion=1`、底部一行按钮 `proportion=0`。

### 3.2 wxGridSizer / wxFlexGridSizer：表格布局

- 文档：`sizer.h`:157（Grid）、170（FlexGrid）  
- `wxGridSizer`：
  - 固定行列数，所有格子尺寸相同；  
  - 适合简单图标、按钮矩阵（如数字键盘）。
- `wxFlexGridSizer`：
  - 行/列可以根据内容自动调整各自宽高；  
  - 可以通过 `AddGrowableRow()` / `AddGrowableCol()` 指定哪些行/列可伸缩（见 `src/common/sizer.cpp`:2135）。

示例（简化自 `samples/layout/layout.cpp`:153）：

```cpp
wxGridSizer* gridsizer = new wxGridSizer(2, 5, 5); // 2 列，水平/垂直间距 5px

gridsizer->Add(new wxStaticText(p, wxID_ANY, "Label"),
               wxSizerFlags().Align(wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL));
gridsizer->Add(new wxTextCtrl(p, wxID_ANY, "Grid sizer demo"),
               wxSizerFlags(1).Align(wxGROW | wxALIGN_CENTER_VERTICAL));

// ...
topsizer->Add(gridsizer,
              wxSizerFlags(1).Expand().DoubleBorder(wxALL));
```

经验：

- 要求所有单元格大小一致且简单，可优先使用 `wxGridSizer`；  
- 需要“某列随窗口宽度伸缩”，用 `wxFlexGridSizer` 并调用 `AddGrowableCol()`。

### 3.3 wxGridBagSizer：带坐标的表格布局

- 文档：`sizer.h`:326  
- 特点：
  - 每个元素显式指定 `(row, col)` 和 `span`；  
  - 类似 HTML table 的 `rowspan` / `colspan`；  
  - 可以动态移动元素，检测交叉（`CheckForIntersection`）。

示例中演示动态隐藏/移动控件（`samples/layout/layout.cpp`:530）：

```cpp
void MyGridBagSizerFrame::OnHideBtn(wxCommandEvent&)
{
    m_gbs->Hide(m_hideTxt);
    m_hideBtn->Disable();
    m_showBtn->Enable();
    m_gbs->Layout();
}
```

使用建议：

- 适合复杂表单/属性页，但要小心坐标管理；  
- 动态移动、隐藏后必须调用 `Layout()`；  
- 在添加前可以用 `CheckForIntersection()` 检查是否与已有项冲突。

### 3.4 wxWrapSizer：自动换行布局

- 文档：`sizer.h`:321  
- 特点：
  - 类似水平/垂直 box sizer，但一行（或一列）放不下时自动换行。  
  - 适合一组标签、复选框等数量不固定的控件。

样例见 `MyWrapSizerFrame`（`samples/layout/layout.cpp`:669）：

```cpp
m_wrapSizer = new wxWrapSizer(wxHORIZONTAL);
// 动态添加 checkbox
m_wrapSizer->Add(new wxCheckBox(m_checkboxParent, wxID_ANY, "Option 1"),
                 wxSizerFlags().Centre().Border());
```

要点：

- wrap sizer 一般嵌在 `wxStaticBoxSizer` / `wxBoxSizer` 里；  
- 动态增删子项后调用 `Layout()` 即可重新换行。

### 3.5 wxStdDialogButtonSizer：标准按钮区布局

- 文档：`sizer.h`:316、`dialog.h`:62  
- 特点：
  - 专门用来按平台规范排列对话框的 OK/Cancel/Help 等按钮；  
  - 内部会根据 ID (`wxID_OK`、`wxID_CANCEL` 等) 决定主按钮和顺序。  
  - `Realize()` 内还会调整 Tab 顺序（见 `src/common/sizer.cpp`:2997）。

典型用法：

```cpp
wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer;
btnSizer->AddButton(new wxButton(this, wxID_OK));
btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
btnSizer->Realize();
topsizer->Add(btnSizer, wxSizerFlags().Expand().Border());
```

在很多对话框中直接使用 `wxDialog::CreateButtonSizer()` 或 `CreateSeparatedButtonSizer()`，内部就是基于 `wxStdDialogButtonSizer` 的逻辑。

### 3.6 其他 sizer

- `wxStaticBoxSizer`：一维布局 + 边框标题，常用于分组配置项。  
- `wxBoxSizer::AddSpacer()` / `AddStretchSpacer()`：  
  - `AddSpacer(px)`：固定空白；  
  - `AddStretchSpacer()`：按 proportion 参与空间分配，经常用于把控件推到一边（见 `ExampleBorderlessRoundedWindow.cpp`:33）。
- `wxSizer::Fit()` / `SetSizeHints()`：  
  - `Fit()`：修改关联窗口尺寸以刚好包住所有子项（见 `windowsizing.h`:105）；  
  - `SetSizeHints(window)`：同时调用 `Fit()` 并设置窗口最小尺寸。

---

## 4. 顶层窗口与容器中的布局模式

### 4.1 Frame + Panel + Sizer 模式（强烈推荐）

示例：布局演示程序主窗口（`samples/layout/layout.cpp`:84）：

1. 创建 `wxFrame` 作为顶层窗口；  
2. 在 frame client 区上放一个 `wxPanel`；  
3. 所有控件都放在 panel 上，并把 sizer 设给 panel；  
4. 用 `topsizer->SetSizeHints(this)` 或 `SetSizerAndFit()` 约束 frame。

优点：

- Panel 负责背景、Tab 顺序、焦点转移等；  
- Frame 专注菜单、工具栏、状态栏；  
- 便于在不同平台上统一 client 区行为（尤其是 WM/主题对边距的影响）。

### 4.2 自绘/特效窗口 + Sizer

项目中的圆角无边框窗口示例（`ExampleBorderlessRoundedWindow.cpp`:33）：

```cpp
wxPanel* panel = new wxPanel(this);
AttachDragHandlers(panel);              // 允许拖拽移动窗口

auto* sizer = new wxBoxSizer(wxVERTICAL);
auto* label = new wxStaticText(panel, wxID_ANY, "...说明文本...");
auto* buttonClose = new wxButton(panel, wxID_ANY, "Close");

sizer->Add(label, 0, wxALIGN_CENTER | wxALL, FromDIP(16, panel));
sizer->AddStretchSpacer();
sizer->Add(buttonClose, 0, wxALIGN_CENTER | wxBOTTOM, FromDIP(16, panel));

panel->SetSizer(sizer);
```

即便窗口形状、边框都自定义，**内部控件仍由 sizer 管理**，这样文本/按钮在高 DPI 和多平台下依然整齐。

### 4.3 对话框布局：SetSizerAndFit + 标准按钮区

通用写法（参考 `wxGenericAboutDialog::Create`：`src/generic/aboutdlgg.cpp`:120、260）：

1. 创建一个内容 panel，并用 sizer 组织各种文本/超链接/折叠面板；  
2. 把内容 panel 塞到最外层 `wxBoxSizer`；  
3. 使用 `CreateButtonSizer(wxOK)` 或 `wxStdDialogButtonSizer` 作为底部按钮区；  
4. 调用 `SetSizerAndFit(topSizer)`。

这样，窗口初始大小和最小大小都由 sizer 自动推导，且按钮布局符合平台规范。

---

## 5. 源码中体现的实用布局技巧

### 5.1 善用 wxSizerFlags 提高可读性

`docs/doxygen/overviews/sizer.h`:270 展示了经典的“旧写法”和基于 `wxSizerFlags` 的新写法对比。  
新写法的优点：

- 参数顺序固定：`wxSizerFlags(proportion).Align(...).Expand().Border()`；  
- 能通过 `.DoubleBorder()`、`.CentreVertical()`、`.Proportion(n)` 直接表达意图；  
- 更容易做全局替换和审查。

建议在新代码中始终使用 `wxSizerFlags`，只在必须兼容老接口时使用原始参数。

### 5.2 动态修改布局：Hide / Show / SetItemMinSize

布局示例中演示了几种常用操作（`samples/layout/layout.cpp`:530 之后）：

- `sizer->Hide(window_or_index)` / `Show(...)`：临时移除某项，但保留结构；  
- `SetItemMinSize(item, newSize)`：在不重建控件的前提下改变项的最小尺寸；  
- 修改后调用 `Layout()`（通常是窗口的 `Layout()`，内部会转到 sizer）。

常见场景：高级选项折叠、表单中某些字段基于勾选项显隐、响应快捷键调整文本框高度等。

### 5.3 使用 Fit / SetSizeHints 控制顶层窗口尺寸

`windowsizing.h`:105 提示：

- `wxSizer::Fit(window)`：把 `window` 尺寸调整到刚好能放下 sizer 的最小需求；  
- `wxTopLevelWindow::SetSizeHints(minW, minH, ...)`：约束用户拖动时的最小尺寸。

示例（`samples/layout/layout.cpp`:407）：

```cpp
SetSizer(main_sizer);
Layout();
GetSizer()->Fit(this); // 或 main_sizer->SetSizeHints(this);
```

建议：

- 对话框统一使用 `SetSizerAndFit()`；  
- 主窗口在布局完成后调用 `SetSizeHints()`，防止被拖得太小导致控件重叠。

### 5.4 将“按钮区”与“主体内容”分离

在滚动自适应对话框里（`dialog.h`:62），`wxDialog` 会专门寻找：

- 一个 `wxStdDialogButtonSizer`，或  
- 一行（水平） `wxBoxSizer` 中的一组标准按钮，

然后自动把按钮区固定在底部，让主体内容在 `wxScrolledWindow` 中滚动。  

实践建议：

- 顶层 sizer 一般写成「主内容 sizer + 按钮 sizer」两段；  
- 按钮统一使用 `wxStdDialogButtonSizer` 或 `CreateButtonSizer()`；  
- 不要把按钮零散地插在内容中间，以免影响后续自动布局适配。

### 5.5 布局嵌套技巧：嵌套 Sizer 与自适应界面

复杂界面通常可以拆成「若干大区块 + 区块内部局部布局」两层来设计。合理的嵌套既能提高可读性，又能让后续改 UI 时更容易定位。典型套路：

- 多区域主界面（左树/列表 + 右详情）：  
  用一个水平 `wxBoxSizer` 做根，将左侧区块设为较小权重（或 `proportion = 0`，右侧 `proportion = 1`）。左右各自再用垂直 `wxBoxSizer` 管理顶部工具栏、中心内容、底部日志等。例如：

```cpp
auto* root = new wxBoxSizer(wxHORIZONTAL);

auto* left = new wxBoxSizer(wxVERTICAL);
left->Add(m_tree,   wxSizerFlags(1).Expand());
left->Add(m_filter, wxSizerFlags().Expand().Border(wxTOP));

auto* right = new wxBoxSizer(wxVERTICAL);
right->Add(m_toolbar, wxSizerFlags().Expand().Border(wxBOTTOM));
right->Add(m_mainView, wxSizerFlags(1).Expand());
right->Add(m_log,      wxSizerFlags().Proportion(0).Expand().Border(wxTOP));

root->Add(left,  wxSizerFlags().Proportion(0).Expand().Border(wxALL, FromDIP(4, this)));
root->Add(right, wxSizerFlags(1).Expand().Border(wxALL, FromDIP(4, this)));

SetSizer(root);
```

- 多列对齐的大表单：  
  适合使用 `wxFlexGridSizer`，将 label 列保持固定宽度，输入控件列设置为可伸缩列（通过 `AddGrowableCol()`）。这样在窗口变宽时，只有右侧输入控件列扩展，视觉效果更自然。

- 需要跨行跨列的“复杂表单”：  
  使用 `wxGridBagSizer`，显式指定 `wxGBPosition(row, col)` 和 `wxGBSpan(rowSpan, colSpan)` 来实现控件占用多行/多列，例如说明文字跨两列、宽按钮跨整行。可以结合 `AddGrowableRow()` / `AddGrowableCol()` 实现“某一行/列随窗口伸缩”。

- 标签云、工具按钮面板等“自动换行”布局：  
  使用 `wxWrapSizer`，在水平方向放置一系列按钮/标签，当宽度不足时自动换行。适用于窗口大小变化大、但不需要精确对齐网格的区域（参考 `MyWrapSizerFrame` 示例）。

设计复杂布局时，建议先用盒状 sizer 把区域划分清楚，再在局部区域按需引入 `wxFlexGridSizer` / `wxGridBagSizer` / `wxWrapSizer`，避免在同一层级混入过多不同类型的 sizer，导致难以调试。

进一步实践经验：

- 一层只做一件事：同一层级的 sizer 要么负责“横向分区”，要么负责“纵向分区”，不要一层里又横又纵；  
- 嵌套层数控制在 3～4 层以内：超过 5 层一般说明可以拆出中间面板或复合控件；  
- 把复用区域封装成自定义控件：例如“搜索栏 + 过滤选项 + 表格”可以封成一个 `MySearchPanel`，内部再用 sizer 嵌套，外层只关心这个控件的最小尺寸和伸缩性。

### 5.6 布局填充技巧：proportion / Expand / Spacer

`proportion` 和 `wxEXPAND` 决定了“谁吃掉多余空间”。几个常用模式：

- 主内容占满，其他控件固定：  

  ```cpp
  auto* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(m_toolbar, 0, wxEXPAND | wxALL, FromDIP(4, this)); // 固定高度
  sizer->Add(m_mainView, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(4, this)); // 纵向伸缩
  sizer->Add(m_status, 0, wxEXPAND | wxALL, FromDIP(4, this)); // 固定高度
  ```

- 使用 `AddStretchSpacer()` 把控件“推到一边”：  

  ```cpp
  auto* row = new wxBoxSizer(wxHORIZONTAL);
  row->Add(m_label);
  row->AddStretchSpacer(1);                    // 吃掉中间空白
  row->Add(m_button, 0, wxALIGN_CENTER_VERTICAL);
  ```

- 表单里只让右侧输入框扩展：  
  在 `wxFlexGridSizer` 里对输入控件所在列 `AddGrowableCol(colIndex)`，并给该列的控件加 `wxEXPAND`。

经验：

- “谁应该变大”就给谁 `proportion > 0` + `wxEXPAND`，其他控件尽量保持 `proportion = 0`；  
- 不要用大量固定像素的 `AddSpacer(px)` 抵消 sizer 逻辑，优先用边距（`Border()`）和 stretch spacer；  
- 如果发现窗口变大时空白出现在奇怪的位置，先检查是否有多项同时设置了 `proportion > 0`。

### 5.7 布局对齐技巧：Align 标志的组合与约束

对齐标志的核心规则（配合 6.2 小节中的“坑位”一起理解）：

- 垂直 `wxBoxSizer` 里只使用“水平对齐”标志：  
  `wxALIGN_LEFT / wxALIGN_RIGHT / wxALIGN_CENTER_HORIZONTAL`；竖直方向由 sizer 决定高度，一般用 `wxEXPAND` 或默认即可。

- 水平 `wxBoxSizer` 里只使用“垂直对齐”标志：  
  `wxALIGN_TOP / wxALIGN_BOTTOM / wxALIGN_CENTER_VERTICAL`；水平方向同样尽量交给 sizer。

- `wxEXPAND` 与对齐的关系：  
  在某个方向使用了 `wxEXPAND`，同方向的对齐标志会被忽略，所以要么“填满”，要么“对齐”，不要两者都要。

常见排布示例：

- 左标签 + 右输入框居中对齐：  

  ```cpp
  auto* row = new wxBoxSizer(wxHORIZONTAL);
  row->Add(m_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4, this));
  row->Add(m_text,  1, wxEXPAND);
  ```

- 一行按钮居中：  

  ```cpp
  auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
  btnRow->AddStretchSpacer();
  btnRow->Add(m_ok, 0, wxRIGHT, FromDIP(4, this));
  btnRow->Add(m_cancel);
  btnRow->AddStretchSpacer();
  ```

如果 Debug 版出现“某些 flags will be ignored”断言，优先检查是否在垂直 sizer 里用了 `wxALIGN_TOP/BOTTOM`，或在水平 sizer 里用了 `wxALIGN_LEFT/RIGHT`，再检查是否和 `wxEXPAND` 冲突。

---

## 6. 常见坑位与调试要点

这一节结合 `src/common/sizer.cpp` 中的断言和样例代码，总结在真实项目中经常踩的坑。

### 6.1 控件父窗口与 sizer 所属窗口不匹配

相关检查逻辑在 `MakeExpectedParentMessage()`：`src/common/sizer.cpp`:221。

规则：

- 某个窗口由 `parent->GetSizer()` 管理时，该窗口必须以 `parent`（或其内部 static box）为 `GetParent()`：  
  - 例如：`new wxButton(panel, ...)` 应该交给 `panel->GetSizer()` 管理；  
  - 如果 `new wxButton(frame, ...)` 却被加入 `panel` 的 sizer，中间父子关系就乱了。

症状：

- Debug 版下出现断言信息：  
  “Windows managed by the sizer associated with the given window must have this window as parent...”  
- 控件位置无法正确更新，尤其是在窗口 resize 或隐藏/显示之后。

规避：

- **始终确保控件的父窗口与所在 sizer 的“宿主窗口”一致**；  
- 常见模式：frame -> panel -> sizer -> 控件，全程都以 panel 为父。

### 6.2 sizer 标志冲突或被忽略

Sizer 实现里有一组针对 flags 的一致性检查（`ASSERT_VALID_SIZER_FLAGS`：`src/common/sizer.cpp`:269），常见问题包括：

- 在垂直 box sizer 上使用垂直对齐标志：

```cpp
// 错误：垂直 sizer 只能使用水平对齐标志
sizer->Add(ctrl, 0, wxALIGN_BOTTOM);
```

- 在水平 box sizer 上使用水平对齐标志：

```cpp
// 错误：水平 sizer 要使用垂直方向的对齐标志
sizer->Add(ctrl, 0, wxALIGN_RIGHT);
```

- 同时使用 `wxEXPAND` 和对齐标志：  
  当没有 `wxSHAPED` 时，`wxEXPAND` 会覆盖对齐标志（见 `src/common/sizer.cpp`:307 之后的检查），导致对齐设置被忽略。

症状：

- Debug 版下出现断言提示：某些 flags will be ignored；  
- 控件位置与预期对齐方式不一致。

建议：

- 垂直 sizer：使用 `wxALIGN_LEFT / CENTER_HORIZONTAL / RIGHT`；  
- 水平 sizer：使用 `wxALIGN_TOP / CENTER_VERTICAL / BOTTOM`；  
- 需要控件填满某一方向时，优先使用 `wxEXPAND`，不要再叠加同方向的对齐标志。

### 6.3 忘记 SetSizer / SetSizerAndFit / Layout

典型错误模式：

- 只创建了 sizer 并调用 `Add()`，但从未 `SetSizer()` 到窗口上；  
- 动态增删控件后未调用 `Layout()`。

症状：

- 窗口内部控件全堆在左上角，不随窗口变化；  
- 新增的控件不显示，或隐藏后的空位不回收。

解决：

- 创建完 sizer 后务必调用：

```cpp
panel->SetSizer(sizer);
// 对话框再用：
SetSizerAndFit(sizer);
```

- 动态修改 sizer 后调用：

```cpp
Layout();          // 通常是窗口方法
// 或 sizer->Layout()，但前者更常见
```

### 6.4 手动 SetSize 与 sizer 管理的冲突

`windowsizing.h`:90 提醒：  
如果某个窗口已经由 sizer 管理，则不应再手动在 `EVT_SIZE` 中反复 `SetSize()` 子控件，否则会与 sizer 算法“打架”，表现为：

- 控件在某些尺寸下抖动或位置跳变；  
- 用户拖动窗口时出现奇怪的闪烁。

解决思路：

- 要么完全交给 sizer 管理，并通过 `proportion/flags/minsize` 控制效果；  
- 要么完全不用 sizer，而是自己算坐标（只推荐在少量强定制窗口中使用）。

### 6.5 动态删除控件与内存管理

Sizer 本身不拥有窗口的生命周期，但会在布局时对已删除窗口进行清理。常见安全做法：

- 删除某个控件前先从 sizer 中 `Detach()` 或删除对应 `wxSizerItem`；  
- 更简单的方式是直接 `delete` 控件，sizer 会在下一次 `Layout()` 时自动跳过。

示例（`MyWrapSizerFrame::OnRemoveCheckbox`：`samples/layout/layout.cpp`:720）：

```cpp
delete m_wrapSizer->GetItem(m_wrapSizer->GetItemCount() - 1)->GetWindow();
Layout(); // 触发布局更新
```

注意：

- 不要一边遍历 sizer 的项一边 `delete` 当前项的窗口，容易漏掉或重复处理；  
- 若需要复杂操作，可以先收集要删的窗口指针列表，再统一删除并 `Layout()`。

### 6.6 使用 wxStdDialogButtonSizer 时漏掉 Realize()

当自己手动构造 `wxStdDialogButtonSizer` 时，如果忘记 `Realize()`：

- 按钮不会按平台规范排序；  
- Tab 顺序也可能异常。

解决：

- 手动构造时最后调用 `btnSizer->Realize()`；  
- 或直接使用 `CreateButtonSizer()`/`CreateSeparatedButtonSizer()` 这些帮助函数，由框架内部调用 `Realize()`。

### 6.7 Sizer 调试：关闭或开启标志检查

`src/common/sizer.cpp` 中通过 `WXSUPPRESS_SIZER_FLAGS_CHECK` 环境变量控制断言（`gs_disableFlagChecks`：162）：

- 默认情况下，Debug 版会在 flag 冲突、父窗口不匹配等情况弹出断言对话框。  
- 如在某些三方库或老代码中断言太多，可以临时设置：

```bash
set WXSUPPRESS_SIZER_FLAGS_CHECK=1   # Windows
export WXSUPPRESS_SIZER_FLAGS_CHECK=1  # *nix
```

不建议在新代码中长期依赖关闭检查；  
**更好的做法是利用这些断言修正布局问题**。

---

## 7. 在本项目中组织 UI 布局的建议

结合当前仓库已有的代码风格和 wxWidgets 官方示例，建议在后续工程中遵循：

1. **一律使用 sizer 布局**  
   - 顶层窗口：`Frame -> Panel -> Sizer -> Controls`；  
   - 对话框：`SetSizerAndFit()` + `wxStdDialogButtonSizer`。

2. **统一使用 wxSizerFlags**  
   - 例如 `.Border(wxALL, FromDIP(8, parent))`、`.Proportion(1).Expand()`；  
   - 这样布局参数读起来接近自然语言，日后重构也更易定位。

3. **复杂区域优先考虑 Grid/FlexGrid/GridBag**  
   - 简单二维表单：`wxGridSizer` 或 `wxFlexGridSizer + AddGrowableCol()`；  
   - 有跨行跨列需求：`wxGridBagSizer`。

4. **动态布局统一通过 sizer 操作实现**  
   - 显隐字段：`Show()/Hide() + Layout()`；  
   - 调整控件占用空间：`SetItemMinSize() + Layout()`。

5. **充分利用示例代码作为模板**  
   - `samples/layout/layout.cpp` 包含几乎所有常见 sizer 用法；  
   - `src/generic/aboutdlgg.cpp` 展示了实际产品级对话框的布局方式；  
   - 项目自有的 `ExampleBorderlessRoundedWindow.cpp` 展示了“特效窗口 + sizer”的组合方式。

按照以上思路，基本可以覆盖项目中从简单对话框到复杂主界面的布局需求，同时在不同平台、高 DPI 场景下保持良好的外观与可维护性。
