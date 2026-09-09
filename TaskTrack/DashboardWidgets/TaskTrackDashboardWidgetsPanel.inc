TaskTrackDashboardPanelCtrl::TaskTrackDashboardPanelCtrl()
{
    SetContent(content_); content_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(7)); project_row_.SetDirection(UiDirection::H).SetGap(DPI(10)).SetInset(0).SetAlignItems(UiCrossAlign::Center); body_.WhenToggleExpanded=[=]{SetExpanded(!expanded_);}; timeline_.WhenDouble=[=]{SetExpanded(!expanded_);};
}
TaskTrackDashboardPanelCtrl& TaskTrackDashboardPanelCtrl::Bind(const TaskTrackDashboardPanel& p,double progress,bool expanded){panel_=&p;dashboard_progress_=progress;expanded_=expanded||p.density==TaskTrackDashboardDensity::Full;RebuildContent();return *this;}
TaskTrackDashboardPanelCtrl& TaskTrackDashboardPanelCtrl::SetExpanded(bool e){if(!panel_)return *this;if(panel_->density==TaskTrackDashboardDensity::Full)e=true;if(expanded_!=e){expanded_=e;RebuildContent();WhenExpansionChanged(panel_->id);}return *this;}
void TaskTrackDashboardPanelCtrl::RebuildContent()
{
    if(!panel_)return;content_.ClearItems();project_row_.ClearItems();SetTitle(panel_->title);String sub=panel_->subtitle;if(!expanded_&&panel_->entries.GetCount()>panel_->preview_limit){if(!sub.IsEmpty())sub<<" · ";sub<<Format("%d records; previewing %d",panel_->entries.GetCount(),panel_->preview_limit);}SetSubTitle(sub);body_.Bind(*panel_,expanded_);
    if(panel_->type==TaskTrackDashboardPanelType::ProjectState){
        state_chart_.ClearSegments().ClearTotal().ClearCenterText();
        double total_weight=0.0, completed_weight=0.0;
        for(const auto& e:panel_->entries){
            if(e.archived||e.progress<0)
                continue;
            double weight=max(0.0001,e.weight);
            double completed=weight*minmax(e.progress,0.0,100.0)/100.0;
            state_chart_.AddSegment(completed,e.title);
            total_weight+=weight;
            completed_weight+=completed;
        }
        state_chart_.SetThickness(DPI(14)).SetSegmentGap(DPI(3));
        if(total_weight>0.0){
            state_chart_.SetTotal(total_weight)
                        .SetCenterText(Format("%.0f%%",100.0*completed_weight/total_weight));
        }
        else
            state_chart_.SetCenterText("State");
        project_row_.Add(state_chart_).Fixed(DPI(112)).MinCross(DPI(112));
        project_row_.Add(body_).Expand(1).MinMain(DPI(260)).MinCross(body_.GetMinSize().cy);
        content_.Add(project_row_).Fit().MinMain(max(DPI(116),body_.GetMinSize().cy)).AlignSelf(UiCrossAlign::Stretch);
    }
    else if(panel_->type==TaskTrackDashboardPanelType::Timeline){ValueArray values;for(const auto& e:panel_->entries)values.Add(TaskTrackDashboardEntryToValue(e));timeline_.SetData(values);content_.Add(timeline_).Fit().MinMain(DPI(94)).AlignSelf(UiCrossAlign::Stretch);if(expanded_)content_.Add(body_).Fit().MinMain(body_.GetMinSize().cy).AlignSelf(UiCrossAlign::Stretch);}
    else content_.Add(body_).Fit().MinMain(body_.GetMinSize().cy).AlignSelf(UiCrossAlign::Stretch);
    content_.RefreshLayout();RefreshLayout();Refresh();
}
