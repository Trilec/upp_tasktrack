TaskTrackRangeSelector& TaskTrackRangeSelector::SetRange(double mn, double mx)
{
    if(mx < mn)
        Swap(mn, mx);
    mn_ = mn;
    mx_ = mx;
    low_ = ClampSnap(low_);
    high_ = ClampSnap(high_);
    if(low_ > high_)
        low_ = high_;
    Refresh();
    return *this;
}

TaskTrackRangeSelector& TaskTrackRangeSelector::SetStep(double step)
{
    step_ = max(step, 0.000001);
    low_ = ClampSnap(low_);
    high_ = ClampSnap(high_);
    Refresh();
    return *this;
}

TaskTrackRangeSelector& TaskTrackRangeSelector::SetValues(double low, double high)
{
    low_ = ClampSnap(low);
    high_ = ClampSnap(high);
    if(low_ > high_)
        Swap(low_, high_);
    Refresh();
    return *this;
}

TaskTrackRangeSelector& TaskTrackRangeSelector::SetUnit(const String& unit)
{
    unit_ = unit;
    Refresh();
    return *this;
}

Size TaskTrackRangeSelector::GetMinSize() const
{
    return Size(DPI(230), DPI(48));
}

double TaskTrackRangeSelector::ClampSnap(double value) const
{
    value = minmax(value, mn_, mx_);
    if(step_ > 0.0) {
        double n = floor(((value - mn_) / step_) + 0.5);
        value = mn_ + n * step_;
    }
    return minmax(value, mn_, mx_);
}

int TaskTrackRangeSelector::XFromValue(double value) const
{
    Rect r = GetSize();
    int left = DPI(10);
    int right = max(left + 1, r.GetWidth() - DPI(10));
    double span = max(0.000001, mx_ - mn_);
    double t = (value - mn_) / span;
    return left + (int)floor(t * (right - left) + 0.5);
}

double TaskTrackRangeSelector::ValueFromX(int x) const
{
    Rect r = GetSize();
    int left = DPI(10);
    int right = max(left + 1, r.GetWidth() - DPI(10));
    double t = (double)(minmax(x, left, right) - left) / (double)(right - left);
    return ClampSnap(mn_ + t * (mx_ - mn_));
}

int TaskTrackRangeSelector::HitThumb(Point p) const
{
    int y = DPI(18);
    int r = DPI(9);
    int lx = XFromValue(low_);
    int hx = XFromValue(high_);
    int dl = abs(p.x - lx) + abs(p.y - y);
    int dh = abs(p.x - hx) + abs(p.y - y);
    if(dl <= r * 2 || dh <= r * 2)
        return dl <= dh ? 0 : 1;
    return abs(p.x - lx) <= abs(p.x - hx) ? 0 : 1;
}

void TaskTrackRangeSelector::SetDraggedValue(Point p, bool fire_changing)
{
    double value = ValueFromX(p.x);
    if(dragging_ == 0)
        low_ = min(value, high_);
    else if(dragging_ == 1)
        high_ = max(value, low_);
    Refresh();
    if(fire_changing)
        WhenChanging();
}

void TaskTrackRangeSelector::Paint(Draw& w)
{
    Rect r = GetSize();
    Color ink = SColorText();
    Color track = Blend(SColorShadow(), SColorFace(), 130);
    Color accent = Color(0, 120, 212);
    int y = DPI(18);
    int left = DPI(10);
    int right = max(left + 1, r.GetWidth() - DPI(10));
    int lx = XFromValue(low_);
    int hx = XFromValue(high_);

    w.DrawRect(left, y - DPI(2), right - left, DPI(4), track);
    w.DrawRect(lx, y - DPI(2), max(1, hx - lx), DPI(4), accent);

    Rect lr = RectC(lx - DPI(6), y - DPI(6), DPI(12), DPI(12));
    Rect hr = RectC(hx - DPI(6), y - DPI(6), DPI(12), DPI(12));
    w.DrawEllipse(lr, selected_ == 0 ? accent : SColorFace(), DPI(2), accent);
    w.DrawEllipse(hr, selected_ == 1 ? accent : SColorFace(), DPI(2), accent);

    Font f = SansSerifZ(9);
    String low_text = DisplayNumber(low_) + unit_;
    String high_text = DisplayNumber(high_) + unit_;
    w.DrawText(left, DPI(30), low_text, f, ink);
    Size hs = GetTextSize(high_text, f);
    w.DrawText(max(left, right - hs.cx), DPI(30), high_text, f, ink);
}

void TaskTrackRangeSelector::LeftDown(Point p, dword)
{
    SetFocus();
    selected_ = dragging_ = HitThumb(p);
    SetCapture();
    SetDraggedValue(p, true);
}

void TaskTrackRangeSelector::LeftUp(Point p, dword)
{
    if(dragging_ >= 0)
        SetDraggedValue(p, false);
    dragging_ = -1;
    ReleaseCapture();
    WhenAction();
}

void TaskTrackRangeSelector::MouseMove(Point p, dword)
{
    if(dragging_ >= 0)
        SetDraggedValue(p, true);
}

void TaskTrackRangeSelector::LostFocus()
{
    dragging_ = -1;
    ReleaseCapture();
    Refresh();
}

bool TaskTrackRangeSelector::Key(dword key, int count)
{
    if(key != K_LEFT && key != K_RIGHT)
        return Ctrl::Key(key, count);
    double delta = (key == K_LEFT ? -step_ : step_) * max(1, count);
    if(selected_ == 0)
        low_ = min(ClampSnap(low_ + delta), high_);
    else
        high_ = max(ClampSnap(high_ + delta), low_);
    Refresh();
    WhenChanging();
    WhenAction();
    return true;
}

// -----------------------------------------------------------------------------
// TaskTrackGradientSelector
// -----------------------------------------------------------------------------

TaskTrackGradientSelector::TaskTrackGradientSelector()
{
    WantFocus();
}

TaskTrackGradientSelector& TaskTrackGradientSelector::SetOptions(const Vector<TaskTrackGradientOption>& options)
{
    options_ = clone(options);
    Refresh();
    return *this;
}

TaskTrackGradientSelector& TaskTrackGradientSelector::SetValue(const String& value)
{
    value_ = value;
    Refresh();
    return *this;
}

Size TaskTrackGradientSelector::GetMinSize() const
{
    int count = max(1, options_.GetCount());
    int cols = min(3, count);
    int rows = (count + cols - 1) / cols;
    return Size(DPI(250), rows * DPI(42));
}

Rect TaskTrackGradientSelector::OptionRect(int index) const
{
    Rect r = GetSize();
    int count = max(1, options_.GetCount());
    int cols = min(3, count);
    int rows = (count + cols - 1) / cols;
    int gap = DPI(5);
    int cell_w = max(DPI(60), (r.GetWidth() - (cols - 1) * gap) / cols);
    int cell_h = max(DPI(36), (r.GetHeight() - (rows - 1) * gap) / rows);
    int row = index / cols;
    int col = index % cols;
    return RectC(col * (cell_w + gap), row * (cell_h + gap), cell_w, cell_h);
}

Color TaskTrackGradientSelector::ParseColor(const String& text, Color fallback) const
{
    String s = text;
