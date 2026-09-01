namespace {
String MapString(const Value& map, const char *key) { Value v=map[key]; return IsNull(v)?String():AsString(v); }
double MapDouble(const Value& map, const char *key, double def=-1.0) { Value v=map[key]; return IsNull(v)||!IsNumber(v)?def:(double)v; }
bool IsCompletionStatus(const String& raw) { String s=ToLower(TrimBoth(raw)); return s=="complete"||s=="completed"||s=="done"||s=="pass"||s=="passed"||s=="verified"||s=="accepted"; }
bool IsActiveStatus(const String& raw) { String s=ToLower(TrimBoth(raw)); return s=="active"||s=="in_progress"||s=="working"||s=="current"; }
bool IsBlockedStatus(const String& raw) { String s=ToLower(TrimBoth(raw)); return s=="blocked"||s=="fail"||s=="failed"||s=="error"||s=="critical"||s=="risk"; }
bool IsWarningStatus(const String& raw) { String s=ToLower(TrimBoth(raw)); return s=="warning"||s=="attention"||s=="partial"||s=="review"; }
String PercentText(double p) { return p < 0.0 ? String() : Format("%.0f%%", p); }
}

TaskTrackTimelineRail::TaskTrackTimelineRail() { WantFocus(); Transparent(); }
TaskTrackTimelineRail& TaskTrackTimelineRail::SetRole(UiRole role) { role_=role; RefreshLayout(); Refresh(); return *this; }
TaskTrackTimelineRail& TaskTrackTimelineRail::SetCustomStyle(const Style& style) { custom_style_=style; has_custom_style_=true; RefreshLayout(); Refresh(); return *this; }
TaskTrackTimelineRail& TaskTrackTimelineRail::ClearCustomStyle() { has_custom_style_=false; RefreshLayout(); Refresh(); return *this; }
const TaskTrackTimelineRail::Style& TaskTrackTimelineRail::GetStyle() const { return EffectiveStyle(); }
TaskTrackTimelineRail::Style& TaskTrackTimelineRail::EditStyle() { if(!has_custom_style_) { custom_style_=ResolveThemeStyle(); has_custom_style_=true; } return custom_style_; }
TaskTrackTimelineRail& TaskTrackTimelineRail::SetFont(Font f) { EditStyle().title_font=f; RefreshLayout(); Refresh(); return *this; }
TaskTrackTimelineRail& TaskTrackTimelineRail::SetNodeSize(int px) { EditStyle().node_size=max(DPI(6),px); RebuildGeometry(); RefreshLayout(); Refresh(); return *this; }
TaskTrackTimelineRail& TaskTrackTimelineRail::SetLineWidth(int px) { EditStyle().line_width=max(1,px); Refresh(); return *this; }
TaskTrackTimelineRail& TaskTrackTimelineRail::SetContentInset(int px) { EditStyle().content_inset=max(0,px); RebuildGeometry(); RefreshLayout(); Refresh(); return *this; }
TaskTrackTimelineRail& TaskTrackTimelineRail::ShowLabels(bool on) { EditStyle().show_labels=on; RefreshLayout(); Refresh(); return *this; }
TaskTrackTimelineRail& TaskTrackTimelineRail::SetStatusColors(Color c,Color a,Color n,Color b,Color m) { Style& s=EditStyle(); s.complete=c;s.active=a;s.next=n;s.blocked=b;s.muted=m;Refresh();return *this; }

TaskTrackTimelineRail::Style TaskTrackTimelineRail::ResolveThemeStyle() const
{
    Style s; UiLabel::Style label=UiTheme::ResolveLabel(role_); UiLabel::Style subtle=UiTheme::ResolveLabel(UiRole::Subtle); UiLabel::Style accent=UiTheme::ResolveLabel(UiRole::Accent); UiGroupPanel::Style panel=UiTheme::ResolveGroupPanel(role_);
    s.title_font=label.font.Bold(); s.subtitle_font=subtle.font; s.line=panel.palette.frame[ST_NORMAL]; s.active=accent.palette.ink[ST_NORMAL]; s.next=Blend(s.active,SColorPaper(),120); s.muted=subtle.palette.ink[ST_DISABLED]; s.selected_frame=accent.palette.ink[ST_NORMAL]; return s;
}
const TaskTrackTimelineRail::Style& TaskTrackTimelineRail::EffectiveStyle() const { if(has_custom_style_) return custom_style_; themed_style_=ResolveThemeStyle(); return themed_style_; }

void TaskTrackTimelineRail::SetData(const Value& value)
{
    data_=value; RebuildNodes();
    if(nodes_.IsEmpty()) selected_index_=-1;
    else if(selected_index_<0 || selected_index_>=nodes_.GetCount()) { selected_index_=0; for(int i=0;i<nodes_.GetCount();++i) if(IsActiveStatus(nodes_[i].status)) { selected_index_=i; break; } }
    RebuildGeometry(); RefreshLayout(); Refresh();
}
void TaskTrackTimelineRail::RebuildNodes()
{
    nodes_.Clear(); if(!data_.Is<ValueArray>()) return; ValueArray a=data_;
    for(int i=0;i<a.GetCount();++i) if(a[i].Is<ValueMap>()) { Node n; n.id=MapString(a[i],"id"); n.title=MapString(a[i],"title"); n.subtitle=MapString(a[i],"subtitle"); n.status=MapString(a[i],"status"); n.progress=MapDouble(a[i],"progress",-1); n.data=a[i]; nodes_.Add(pick(n)); }
}
TaskTrackTimelineRail& TaskTrackTimelineRail::SetSelectedIndex(int index,bool fire)
{
    int next=nodes_.IsEmpty()?-1:minmax(index,0,nodes_.GetCount()-1); if(next!=selected_index_) { selected_index_=next; Refresh(); if(fire&&selected_index_>=0) WhenSelect(selected_index_); } return *this;
}
Size TaskTrackTimelineRail::GetMinSize() const { const Style& s=EffectiveStyle(); return Size(DPI(320),s.content_inset*2+s.node_size+(s.show_labels?DPI(52):0)+s.label_gap); }
void TaskTrackTimelineRail::Layout() { RebuildGeometry(); }
void TaskTrackTimelineRail::RebuildGeometry()
{
    if(nodes_.IsEmpty()) return; const Style& s=EffectiveStyle(); Size sz=GetSize(); int left=s.content_inset+s.node_size/2+s.selected_extra; int right=max(left,sz.cx-s.content_inset-s.node_size/2-s.selected_extra); int y=s.content_inset+s.node_size/2+s.selected_extra; int span=max(1,right-left);
    for(int i=0;i<nodes_.GetCount();++i) { int x=nodes_.GetCount()==1?left:left+i*span/(nodes_.GetCount()-1); nodes_[i].center=Point(x,y); int hit=max(DPI(20),s.node_size+s.selected_extra*2); nodes_[i].hit=RectC(x-hit/2,y-hit/2,hit,hit+DPI(30)); }
}
Color TaskTrackTimelineRail::NodeColor(const Node& n,const Style& s) const { if(IsCompletionStatus(n.status))return s.complete;if(IsActiveStatus(n.status))return s.active;if(IsBlockedStatus(n.status))return s.blocked;if(IsWarningStatus(n.status))return Blend(s.blocked,s.next,100);String x=ToLower(TrimBoth(n.status));if(x=="next"||x=="planned"||x=="pending"||x=="queued")return s.next;return s.muted; }
