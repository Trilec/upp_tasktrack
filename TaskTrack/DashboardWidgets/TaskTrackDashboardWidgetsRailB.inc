void TaskTrackTimelineRail::Paint(Draw& w)
{
    const Style& s=EffectiveStyle(); Size sz=GetSize(); if(nodes_.IsEmpty()) { w.DrawText(s.content_inset,s.content_inset,"No timeline entries",s.subtitle_font,s.muted); return; }
    RebuildGeometry(); int y=nodes_[0].center.y; if(nodes_.GetCount()>1) w.DrawLine(nodes_[0].center.x,y,nodes_.Top().center.x,y,max(1,s.line_width),s.line);
    int count=nodes_.GetCount(), available=max(1,sz.cx-s.content_inset*2), label_width=max(DPI(68),available/max(1,min(count,5))-DPI(8)); bool sparse=count>7;
    for(int i=0;i<count;++i) { const Node& n=nodes_[i]; Color color=NodeColor(n,s); int extra=i==selected_index_?s.selected_extra:0; int d=max(DPI(6),s.node_size+extra); int x=n.center.x-d/2, ny=n.center.y-d/2; w.DrawEllipse(x,ny,d,d,color);
        if(i==selected_index_||i==hover_index_) { int o=i==selected_index_?DPI(2):DPI(1); Color oc=i==selected_index_?s.selected_frame:s.line; w.DrawEllipse(x-o,ny-o,d+o*2,d+o*2,Null,max(1,DPI(1)),oc); }
        if(has_focus_&&i==selected_index_) w.DrawEllipse(x-DPI(4),ny-DPI(4),d+DPI(8),d+DPI(8),Null,max(1,DPI(1)),s.selected_frame);
        if(s.show_labels&&(!sparse||i==selected_index_||IsActiveStatus(n.status)||i==0||i==count-1)) { String title=n.title; while(title.GetCount()>4&&GetTextSize(title,s.title_font).cx>label_width) title=title.Left(title.GetCount()-2); if(title!=n.title) title<<"…"; int tx=minmax(n.center.x-label_width/2,s.content_inset,max(s.content_inset,sz.cx-s.content_inset-label_width)); int ty=n.center.y+d/2+s.label_gap; w.DrawText(tx,ty,title,s.title_font,i==selected_index_?s.selected_frame:SColorText()); String sub=n.subtitle; if(sub.IsEmpty())sub=n.status; if(n.progress>=0){if(!sub.IsEmpty())sub<<" · ";sub<<PercentText(n.progress);} if(!sub.IsEmpty()) { while(sub.GetCount()>4&&GetTextSize(sub,s.subtitle_font).cx>label_width) sub=sub.Left(sub.GetCount()-2); w.DrawText(tx,ty+GetTextSize("Ag",s.title_font).cy+DPI(1),sub,s.subtitle_font,s.muted); } }
    }
}
int TaskTrackTimelineRail::HitTest(Point p) const { for(int i=0;i<nodes_.GetCount();++i) if(nodes_[i].hit.Contains(p)) return i; return -1; }
void TaskTrackTimelineRail::LeftDown(Point p,dword) { SetFocus(); int i=HitTest(p); if(i>=0)SetSelectedIndex(i,true); }
void TaskTrackTimelineRail::LeftDouble(Point p,dword) { int i=HitTest(p); if(i>=0)SetSelectedIndex(i,true); WhenDouble(); }
void TaskTrackTimelineRail::MouseMove(Point p,dword) { int n=HitTest(p); if(n!=hover_index_){hover_index_=n;Refresh();} }
void TaskTrackTimelineRail::MouseLeave(){hover_index_=-1;Refresh();}
bool TaskTrackTimelineRail::Key(dword key,int){if(nodes_.IsEmpty())return false;if(key==K_LEFT){SetSelectedIndex(max(0,selected_index_-1),true);return true;}if(key==K_RIGHT){SetSelectedIndex(min(nodes_.GetCount()-1,selected_index_+1),true);return true;}if(key==K_HOME){SetSelectedIndex(0,true);return true;}if(key==K_END){SetSelectedIndex(nodes_.GetCount()-1,true);return true;}if(key==K_ENTER||key==K_SPACE){WhenDouble();return true;}return false;}
void TaskTrackTimelineRail::GotFocus(){has_focus_=true;Refresh();}
void TaskTrackTimelineRail::LostFocus(){has_focus_=false;Refresh();}
