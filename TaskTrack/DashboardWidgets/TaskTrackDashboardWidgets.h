#ifndef _TaskTrack_DashboardWidgets_TaskTrackDashboardWidgets_h_
#define _TaskTrack_DashboardWidgets_TaskTrackDashboardWidgets_h_

/* TaskTrack native semantic dashboard renderers. */

#include <Ui/Ui.h>
#include <TaskTrack/DashboardCore/TaskTrackDashboardCore.h>

namespace Upp {

class TaskTrackTimelineRail : public Ctrl {
public:
    typedef TaskTrackTimelineRail CLASSNAME;

    struct Style : ChStyle<Style> {
        Font title_font = SansSerifZ(9).Bold();
        Font subtitle_font = SansSerifZ(8);
        Color line = SColorShadow();
        Color complete = Color(45, 142, 77);
        Color active = SColorHighlight();
        Color next = Blend(SColorHighlight(), SColorPaper(), 120);
        Color blocked = Color(190, 70, 60);
        Color muted = SColorDisabled();
        Color selected_frame = SColorHighlight();
        int line_width = DPI(2);
        int node_size = DPI(14);
        int selected_extra = DPI(4);
        int content_inset = DPI(10);
        int label_gap = DPI(7);
        bool show_labels = true;
        void Serialize(Stream& s)
        {
            s % title_font % subtitle_font % line % complete % active % next
              % blocked % muted % selected_frame % line_width % node_size
              % selected_extra % content_inset % label_gap % show_labels;
        }
    };

    TaskTrackTimelineRail();
    TaskTrackTimelineRail& SetRole(UiRole role);
    UiRole GetRole() const { return role_; }
    TaskTrackTimelineRail& SetCustomStyle(const Style& style);
    TaskTrackTimelineRail& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const;
    const Style& GetCustomStyle() const { return custom_style_; }

    TaskTrackTimelineRail& SetFont(Font title);
    TaskTrackTimelineRail& SetNodeSize(int px);
    TaskTrackTimelineRail& SetLineWidth(int px);
    TaskTrackTimelineRail& SetContentInset(int px);
    TaskTrackTimelineRail& ShowLabels(bool on = true);
    TaskTrackTimelineRail& SetStatusColors(Color complete, Color active, Color next,
                                           Color blocked, Color muted);

    TaskTrackTimelineRail& SetSelectedIndex(int index, bool fire_event = false);
    int GetSelectedIndex() const { return selected_index_; }
    Value GetSelectedData() const { return selected_index_ >= 0 && selected_index_ < nodes_.GetCount() ? nodes_[selected_index_].data : Value(); }
    int GetNodeCount() const { return nodes_.GetCount(); }

    virtual void SetData(const Value& value) override;
    virtual Value GetData() const override { return data_; }
    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w) override;
    virtual void Layout() override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void LeftDouble(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual bool Key(dword key, int count) override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;

    Event<int> WhenSelect;
    Event<> WhenDouble;

private:
    struct Node : Moveable<Node> {
        String id, title, subtitle, status;
        double progress = -1.0;
        Value data;
        Point center;
        Rect hit;
    };

    Style ResolveThemeStyle() const;
    const Style& EffectiveStyle() const;
    Style& EditStyle();
    void RebuildNodes();
    void RebuildGeometry();
    int HitTest(Point p) const;
    Color NodeColor(const Node& node, const Style& style) const;

    Value data_;
    Vector<Node> nodes_;
    mutable Style themed_style_;
    Style custom_style_;
    bool has_custom_style_ = false;
    UiRole role_ = UiRole::Standard;
    int selected_index_ = -1;
    int hover_index_ = -1;
    bool has_focus_ = false;
};

class TaskTrackDashboardBodyCtrl : public Ctrl {
public:
    typedef TaskTrackDashboardBodyCtrl CLASSNAME;
    TaskTrackDashboardBodyCtrl();
    TaskTrackDashboardBodyCtrl& Bind(const TaskTrackDashboardPanel& panel, bool expanded = false);
    TaskTrackDashboardBodyCtrl& SetExpanded(bool expanded);
    bool IsExpanded() const { return expanded_; }
    virtual Size GetMinSize() const override;
    virtual void Paint(Draw& w) override;
    virtual void LeftDouble(Point p, dword flags) override;
    Event<> WhenToggleExpanded;
private:
    int VisibleCount() const;
    int RowHeight() const;
    String FitText(const String& text, Font font, int max_width) const;
    Color StatusColor(const String& status, bool attention) const;
    const TaskTrackDashboardPanel *panel_ = nullptr;
    bool expanded_ = false;
};

class TaskTrackDashboardPanelCtrl : public UiGroupPanel {
public:
    typedef TaskTrackDashboardPanelCtrl CLASSNAME;
    TaskTrackDashboardPanelCtrl();
    TaskTrackDashboardPanelCtrl& Bind(const TaskTrackDashboardPanel& panel,
                                      double dashboard_progress,
                                      bool expanded = false);
    TaskTrackDashboardPanelCtrl& SetExpanded(bool expanded);
    bool IsExpanded() const { return expanded_; }
    String GetPanelId() const { return panel_ ? panel_->id : String(); }
    Event<const String&> WhenExpansionChanged;
private:
    void RebuildContent();
    const TaskTrackDashboardPanel *panel_ = nullptr;
    double dashboard_progress_ = -1.0;
    bool expanded_ = false;
    UiBoxLayout content_ { UiDirection::V };
    UiBoxLayout project_row_ { UiDirection::H };
    UiProgressRing progress_ring_;
    TaskTrackDashboardBodyCtrl body_;
    TaskTrackTimelineRail timeline_;
};

} // namespace Upp
#endif
