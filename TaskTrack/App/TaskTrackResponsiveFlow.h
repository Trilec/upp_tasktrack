#ifndef _TaskTrack_App_TaskTrackResponsiveFlow_h_
#define _TaskTrack_App_TaskTrackResponsiveFlow_h_

#include <Ui/Ui.h>

namespace Upp {

// Responsive flow used by the category strip. Buttons stay compact and equal
// within the current row and wrap deterministically when width becomes tight.
class TaskTrackCategoryFlow : public UiBoxLayout {
public:
    TaskTrackCategoryFlow(UiDirection d = UiDirection::H)
        : UiBoxLayout(d)
    {
    }

    TaskTrackCategoryFlow& SetDirection(UiDirection d)
    {
        UiBoxLayout::SetDirection(d);
        return *this;
    }

    TaskTrackCategoryFlow& SetGap(int x, int y)
    {
        gap_x_ = max(DPI(6), x);
        gap_y_ = max(DPI(6), y);
        UiBoxLayout::SetGap(gap_x_, gap_y_);
        return *this;
    }

    TaskTrackCategoryFlow& SetInset(int all)
    {
        inset_ = max(0, all);
        UiBoxLayout::SetInset(inset_);
        return *this;
    }

    int DesiredHeightForWidth(int total_width) const
    {
        int count = GetItemCount();
        if(count <= 0)
            return 0;
        int available = max(1, total_width - inset_ * 2);
        int columns = ResolveColumns(available);
        int rows = (count + columns - 1) / columns;
        return inset_ * 2 + rows * DPI(26) + max(0, rows - 1) * gap_y_;
    }

    void Layout() override
    {
        if(!adapting_ && GetItemCount() > 0) {
            int available = max(1, GetSize().cx - inset_ * 2);
            int columns = ResolveColumns(available);
            int width = ResolveItemWidth(available, columns);

            adapting_ = true;
            PauseLayout();
            UiBoxLayout::SetFixedColumn(0);
            for(int i = 0; i < GetItemCount(); ++i)
                ItemAt(i).MinMaxMain(width, width).Fit().MinCross(DPI(26)).AlignSelf(UiCrossAlign::Center);
            ResumeLayout(false);
            adapting_ = false;
        }
        UiBoxLayout::Layout();
    }

private:
    int ResolveColumns(int available) const
    {
        int count = max(1, GetItemCount());
        return min(count, max(1, (available + gap_x_) / (DPI(120) + gap_x_)));
    }

    int ResolveItemWidth(int available, int columns) const
    {
        int width = columns > 0 ? (available - gap_x_ * max(0, columns - 1)) / columns : DPI(120);
        return minmax(width, DPI(120), DPI(180));
    }

    int gap_x_ = DPI(6);
    int gap_y_ = DPI(6);
    int inset_ = 0;
    bool adapting_ = false;
};

// Keep the normal UiGroupPanel chrome, but size the category body from the
// actual current shell width. The previous approximation under-counted the
// header/body chrome by a few pixels and could clip the wrapped button row.
class TaskTrackCategoryPanel : public UiGroupPanel {
public:
    TaskTrackCategoryPanel& SetContent(Ctrl& ctrl)
    {
        flow_ = dynamic_cast<TaskTrackCategoryFlow *>(&ctrl);
        UiGroupPanel::SetContent(ctrl);
        return *this;
    }

    Size GetMinSize() const override
    {
        int width = GetSize().cx;
        if(width <= 0 && GetParent())
            width = GetParent()->GetSize().cx;
        width = max(DPI(160), width);

        const UiGroupPanel::Style& style = GetStyle();
        int flow_width = max(1, width - style.inset.left - style.inset.right);
        int body_height = flow_ ? flow_->DesiredHeightForWidth(flow_width) : DPI(26);
        int title_height = GetTextSize("Categories", style.title_font).cy;
        int header_height = style.header_inset.top + title_height + style.header_inset.bottom;
        int content_height = header_height + style.header_gap
                           + style.inset.top + body_height + style.inset.bottom
                           + DPI(2); // breathing room against rounded/frame edges
        Size outer = UiStyledOuterSizeFromContent(Size(DPI(160), content_height),
                                                  style.metrics, style.skin);
        return Size(DPI(160), max(DPI(56), outer.cy));
    }

private:
    TaskTrackCategoryFlow *flow_ = nullptr;
};

// Responsive question workspace. Every row is an ordered grid-like flow with
// one deterministic card width, at most three columns, and equal row height.
// The last partial row keeps the same column width instead of stretching into
// unrelated masonry widths.
class TaskTrackQuestionFlow : public UiBoxLayout {
public:
    TaskTrackQuestionFlow(UiDirection d = UiDirection::H)
        : UiBoxLayout(d)
    {
    }

    TaskTrackQuestionFlow& SetDirection(UiDirection d)
    {
        UiBoxLayout::SetDirection(d);
        return *this;
    }

    TaskTrackQuestionFlow& SetGap(int x, int y)
    {
        gap_x_ = max(DPI(10), x);
        gap_y_ = max(DPI(10), y);
        UiBoxLayout::SetGap(gap_x_, gap_y_);
        return *this;
    }

    TaskTrackQuestionFlow& SetInset(int all)
    {
        inset_ = max(0, all);
        UiBoxLayout::SetInset(inset_);
        return *this;
    }

    void Layout() override
    {
        if(!adapting_ && GetItemCount() > 0) {
            int available = max(1, GetSize().cx - inset_ * 2);
            int minimum = DPI(280);
            int columns = min(3, max(1, (available + gap_x_) / (minimum + gap_x_)));
            int width = columns > 0 ? (available - gap_x_ * max(0, columns - 1)) / columns : minimum;
            width = max(DPI(220), width);

            adapting_ = true;
            PauseLayout();
            UiBoxLayout::SetFixedColumn(0);
            for(int i = 0; i < GetItemCount(); ++i)
                ItemAt(i).MinMaxMain(width, width).Fit().MinCross(DPI(92)).AlignSelf(UiCrossAlign::Stretch);
            ResumeLayout(false);
            adapting_ = false;
        }
        UiBoxLayout::Layout();
    }

private:
    int gap_x_ = DPI(10);
    int gap_y_ = DPI(10);
    int inset_ = 0;
    bool adapting_ = false;
};

} // namespace Upp

#endif
