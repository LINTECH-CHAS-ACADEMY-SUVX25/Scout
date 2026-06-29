#pragma once

class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual bool init() = 0;
    virtual void drawBitmap(int x1, int y1, int x2, int y2, const void *buf) = 0;
};
