#include "eyerelax/Icons.hpp"
#include "eyerelax/ResourcePath.hpp"

#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace {
constexpr int kIconSize = 32;

wxIcon CreateFallbackEyeIcon(bool open) {
    const wxColour transparent(255, 0, 255);
    wxBitmap bitmap(16, 16, 24);
    wxMemoryDC dc(bitmap);
    dc.SetBackground(wxBrush(transparent));
    dc.Clear();

    if (open) {
        const wxPoint eye[] = {
            {1, 8}, {4, 4}, {8, 3}, {12, 4}, {15, 8},
            {12, 12}, {8, 13}, {4, 12}
        };
        dc.SetPen(wxPen(wxColour(35, 92, 150), 1));
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawPolygon(8, eye);
        dc.SetPen(wxPen(wxColour(45, 115, 55), 1));
        dc.SetBrush(wxBrush(wxColour(94, 181, 78)));
        dc.DrawCircle(8, 8, 3);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(wxColour(20, 45, 65)));
        dc.DrawCircle(8, 8, 1);
    } else {
        dc.SetPen(wxPen(wxColour(95, 105, 115), 2));
        dc.DrawLine(2, 6, 5, 9);
        dc.DrawLine(5, 9, 8, 10);
        dc.DrawLine(8, 10, 11, 9);
        dc.DrawLine(11, 9, 14, 6);
        dc.SetPen(wxPen(wxColour(125, 133, 140), 1));
        dc.DrawLine(4, 9, 3, 12);
        dc.DrawLine(7, 10, 7, 13);
        dc.DrawLine(10, 10, 11, 13);
    }
    dc.SelectObject(wxNullBitmap);
    bitmap.SetMask(new wxMask(bitmap, transparent));
    wxImage fallback = bitmap.ConvertToImage();
    fallback.Rescale(kIconSize, kIconSize, wxIMAGE_QUALITY_HIGH);
    wxIcon icon;
    icon.CopyFromBitmap(wxBitmap(fallback));
    return icon;
}
} // namespace

wxIcon CreateEyeIcon(bool open) {
    const wxString path = ResourcePath(open ? "icons/eye-open.png" : "icons/eye-closed.png");
    wxImage image;
    if (image.LoadFile(path, wxBITMAP_TYPE_PNG)) {
        image.Rescale(kIconSize, kIconSize, wxIMAGE_QUALITY_HIGH);
        wxIcon icon;
        icon.CopyFromBitmap(wxBitmap(image));
        if (icon.IsOk()) return icon;
    }
    return CreateFallbackEyeIcon(open);
}
