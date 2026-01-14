#include <windows.h>
#include <windowsx.h>
#include <cmath>
#include <vector>

int canvas_width = 800;
int canvas_height = 600;
const int SIDEBAR_WIDTH = 250;

struct Point2D {
    double x, y;
    Point2D(double x = 0, double y = 0) : x(x), y(y) {}
};

enum ShapeType {
    SHAPE_CIRCLE
};

struct Shape {
    ShapeType type;
    Point2D center;
    double size1, size2;
    bool is_light;
    
    Shape(ShapeType t, Point2D c, double s1, double s2 = 0, bool light = false) 
        : type(t), center(c), size1(s1), size2(s2), is_light(light) {}
    
    bool intersects_ray(Point2D from, Point2D direction, double& t);
    bool contains_point(Point2D p);
    void draw(HDC hdc);
};

Point2D light_pos(150, 400);
std::vector<Shape> shapes;
ShapeType selected_shape = SHAPE_CIRCLE;
int selected_shape_index = -1;  // which shape we're currently editing
bool dragging_light = false;
bool dragging_shape = false;
bool resizing_shape = false;
Point2D drag_offset;

bool Shape::intersects_ray(Point2D from, Point2D direction, double& t) {
    // checking if a ray hits our circle
    double dx = from.x - center.x;
    double dy = from.y - center.y;
    double a = direction.x * direction.x + direction.y * direction.y;
    double b = 2 * (dx * direction.x + dy * direction.y);
    double c = dx * dx + dy * dy - size1 * size1;
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;
    double sqrt_d = std::sqrt(discriminant);
    double t1 = (-b - sqrt_d) / (2 * a);
    if (t1 > 0.001) { t = t1; return true; }
    double t2 = (-b + sqrt_d) / (2 * a);
    if (t2 > 0.001) { t = t2; return true; }
    return false;
}

bool Shape::contains_point(Point2D p) {
    // is the point inside our circle?
    double dx = p.x - center.x, dy = p.y - center.y;
    return (dx * dx + dy * dy) <= (size1 * size1);
}

bool check_collision_with_shapes(Point2D pos, double size, int exclude_index = -1) {
    for (int i = 0; i < shapes.size(); i++) {
        if (i == exclude_index || shapes[i].is_light) continue;
        
        double dx = pos.x - shapes[i].center.x;
        double dy = pos.y - shapes[i].center.y;
        double dist = std::sqrt(dx * dx + dy * dy);
        double min_dist = size + (shapes[i].type == SHAPE_CIRCLE ? shapes[i].size1 : 
                                  std::max(shapes[i].size1, shapes[i].size2) * 0.7);
        
        if (dist < min_dist) return true;
    }
    return false;
}

void Shape::draw(HDC hdc) {
    // drawing a simple circle
    HBRUSH brush = CreateSolidBrush(RGB(220, 220, 220));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    SelectObject(hdc, brush);
    SelectObject(hdc, pen);
    Ellipse(hdc, (int)(center.x - size1), (int)(center.y - size1),
                 (int)(center.x + size1), (int)(center.y + size1));
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawSidebar(HDC hdc, int height) {
    // drawing the control panel on the right side
    RECT sidebar = {canvas_width, 0, canvas_width + SIDEBAR_WIDTH, height};
    HBRUSH dark_brush = CreateSolidBrush(RGB(30, 30, 35));
    FillRect(hdc, &sidebar, dark_brush);
    DeleteObject(dark_brush);
    
    // a nice accent line on the left edge
    HPEN border_pen = CreatePen(PS_SOLID, 2, RGB(100, 120, 255));
    SelectObject(hdc, border_pen);
    MoveToEx(hdc, canvas_width, 0, NULL);
    LineTo(hdc, canvas_width, height);
    DeleteObject(border_pen);
    
    // the title card at the top
    int card_margin = 20;
    int card_y = 30;
    int card_height = 80;
    RECT card_rect = {canvas_width + card_margin, card_y, canvas_width + SIDEBAR_WIDTH - card_margin, card_y + card_height};
    
    // making it look modern with a card background
    HBRUSH card_brush = CreateSolidBrush(RGB(45, 45, 55));
    FillRect(hdc, &card_rect, card_brush);
    DeleteObject(card_brush);
    
    // subtle border around the card
    HPEN card_pen = CreatePen(PS_SOLID, 1, RGB(70, 70, 80));
    SelectObject(hdc, card_pen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, card_rect.left, card_rect.top, card_rect.right, card_rect.bottom);
    DeleteObject(card_pen);
    
    // the title text with a nice font
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    HFONT title_font = CreateFont(24, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, 0, 0, 0, 0, 0, "Segoe UI");
    SelectObject(hdc, title_font);
    RECT title_rect = {card_rect.left + 15, card_rect.top + 15, card_rect.right - 15, card_rect.bottom - 15};
    DrawText(hdc, "Add Circle", -1, &title_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(title_font);
    
    // another card for instructions below
    int info_card_y = card_y + card_height + 20;
    RECT info_card_rect = {canvas_width + card_margin, info_card_y, canvas_width + SIDEBAR_WIDTH - card_margin, height - 30};
    
    // darker background for the info card
    HBRUSH info_card_brush = CreateSolidBrush(RGB(40, 40, 48));
    FillRect(hdc, &info_card_rect, info_card_brush);
    DeleteObject(info_card_brush);
    
    // border for the info card
    HPEN info_card_pen = CreatePen(PS_SOLID, 1, RGB(60, 60, 70));
    SelectObject(hdc, info_card_pen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, info_card_rect.left, info_card_rect.top, info_card_rect.right, info_card_rect.bottom);
    DeleteObject(info_card_pen);
    
    // helpful instructions for the user
    SetTextColor(hdc, RGB(200, 200, 210));
    HFONT info_font = CreateFont(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "Segoe UI");
    SelectObject(hdc, info_font);
    RECT text_rect = {info_card_rect.left + 20, info_card_rect.top + 20, info_card_rect.right - 20, info_card_rect.bottom - 20};
    DrawText(hdc, "Left click: Add circle\n\n Drag light or circle to move\n\n Drag blue handle to resize\n\n Right click: Delete circle", -1, &text_rect, 
             DT_LEFT | DT_WORDBREAK);
    DeleteObject(info_font);
}

void Render(HDC hdc, int total_width, int height) {
    // black background for the canvas
    RECT canvas = {0, 0, canvas_width, height};
    FillRect(hdc, &canvas, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    // drawing a subtle grid
    HPEN grid_pen = CreatePen(PS_SOLID, 1, RGB(40, 40, 40));
    SelectObject(hdc, grid_pen);
    for (int x = 0; x < canvas_width; x += 20) {
        MoveToEx(hdc, x, 0, NULL);
        LineTo(hdc, x, height);
    }
    for (int y = 0; y < height; y += 20) {
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, canvas_width, y);
    }
    DeleteObject(grid_pen);
    
    // casting rays from the light source
    HPEN ray_pen = CreatePen(PS_SOLID, 1, RGB(255, 240, 100));
    SelectObject(hdc, ray_pen);
    for (int i = 0; i < 360; i++) {
        double angle = i * 3.14159265359 / 180.0;
        Point2D dir(std::cos(angle), std::sin(angle));
        double min_t = 10000;
        bool hit = false;
        
        for (auto& shape : shapes) {
            if (shape.is_light) continue;
            double t;
            if (shape.intersects_ray(light_pos, dir, t) && t < min_t) {
                min_t = t;
                hit = true;
            }
        }
        
        Point2D end = hit ? Point2D(light_pos.x + dir.x * min_t, light_pos.y + dir.y * min_t)
                          : Point2D(light_pos.x + dir.x * 2000, light_pos.y + dir.y * 2000);
        MoveToEx(hdc, (int)light_pos.x, (int)light_pos.y, NULL);
        LineTo(hdc, (int)end.x, (int)end.y);
    }
    DeleteObject(ray_pen);
    
    // drawing all the shapes on screen
    for (int i = 0; i < shapes.size(); i++) {
        auto& shape = shapes[i];
        if (shape.is_light) {
            // making the light source glow nicely
            for (int j = 5; j > 0; j--) {
                HBRUSH glow = CreateSolidBrush(RGB(255, 240 - j*20, 100 - j*15));
                HPEN glow_pen = CreatePen(PS_SOLID, 1, RGB(255, 240 - j*20, 100 - j*15));
                SelectObject(hdc, glow);
                SelectObject(hdc, glow_pen);
                Ellipse(hdc, (int)(shape.center.x - shape.size1 - j*8), 
                             (int)(shape.center.y - shape.size1 - j*8),
                             (int)(shape.center.x + shape.size1 + j*8), 
                             (int)(shape.center.y + shape.size1 + j*8));
                DeleteObject(glow);
                DeleteObject(glow_pen);
            }
            // the bright center of the light
            HBRUSH light_brush = CreateSolidBrush(RGB(255, 255, 255));
            SelectObject(hdc, light_brush);
            Ellipse(hdc, (int)(shape.center.x - shape.size1), (int)(shape.center.y - shape.size1),
                         (int)(shape.center.x + shape.size1), (int)(shape.center.y + shape.size1));
            DeleteObject(light_brush);
        } else {
            shape.draw(hdc);
            
            // showing which shape is selected
            if (i == selected_shape_index) {
                HPEN select_pen = CreatePen(PS_DOT, 2, RGB(100, 200, 255));
                SelectObject(hdc, select_pen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                
                // dotted outline around selected shape
                Ellipse(hdc, (int)(shape.center.x - shape.size1 - 5), 
                             (int)(shape.center.y - shape.size1 - 5),
                             (int)(shape.center.x + shape.size1 + 5), 
                             (int)(shape.center.y + shape.size1 + 5));
                
                // blue handle for resizing
                HBRUSH handle_brush = CreateSolidBrush(RGB(100, 200, 255));
                SelectObject(hdc, handle_brush);
                int hx = (int)(shape.center.x + shape.size1);
                int hy = (int)shape.center.y;
                Ellipse(hdc, hx - 6, hy - 6, hx + 6, hy + 6);
                DeleteObject(handle_brush);
                DeleteObject(select_pen);
            }
        }
    }
    
    // adding the sidebar interface
    DrawSidebar(hdc, height);
    
    // my signature in the corner
    SetBkMode(hdc, TRANSPARENT);
    HFONT credit_font = CreateFontA(32, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "Segoe UI");
    SelectObject(hdc, credit_font);
    
    const char* credit_text = "Made by Batuhan Eroglu";
    SIZE text_size;
    GetTextExtentPoint32A(hdc, credit_text, strlen(credit_text), &text_size);
    
    // keeping it in the bottom-left corner
    int text_x = 15;
    int text_y = height - text_size.cy - 15;
    
    // shadow for better readability
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOutA(hdc, text_x + 2, text_y + 2, credit_text, strlen(credit_text));
    
    // the actual text
    SetTextColor(hdc, RGB(255, 255, 255));
    TextOutA(hdc, text_x, text_y, credit_text, strlen(credit_text));
    
    DeleteObject(credit_font);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    auto is_on_resize_handle = [](Point2D p, Shape& shape) -> bool {
        // checking if mouse is on the resize handle
        int hx = (int)(shape.center.x + shape.size1);
        int hy = (int)shape.center.y;
        return (p.x - hx)*(p.x - hx) + (p.y - hy)*(p.y - hy) <= 36;
    };
    
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            Point2D click(x, y);
            
            // ignoring clicks on the sidebar
            if (x >= canvas_width) return 0;
            
            // checking if we're clicking the resize handle
            if (selected_shape_index >= 0) {
                if (is_on_resize_handle(click, shapes[selected_shape_index])) {
                    resizing_shape = true;
                    SetCapture(hwnd);
                    return 0;
                }
            }
            
            // checking if we clicked on the light
            double dx = x - light_pos.x, dy = y - light_pos.y;
            if (dx * dx + dy * dy < 900) {
                dragging_light = true;
                selected_shape_index = -1;
                SetCapture(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            
            // checking if we clicked on any shape
            bool found = false;
            for (int i = shapes.size() - 1; i >= 0; i--) {
                if (!shapes[i].is_light && shapes[i].contains_point(click)) {
                    selected_shape_index = i;
                    dragging_shape = true;
                    drag_offset.x = x - shapes[i].center.x;
                    drag_offset.y = y - shapes[i].center.y;
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                // creating a new circle if there's space
                double size = 50;
                
                if (!check_collision_with_shapes(click, size)) {
                    shapes.push_back(Shape(SHAPE_CIRCLE, click, 50));
                    selected_shape_index = shapes.size() - 1;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            return 0;
        }
        
        case WM_RBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            
            if (x < canvas_width) {
                Point2D click(x, y);
                for (int i = shapes.size() - 1; i >= 0; i--) {
                    if (!shapes[i].is_light && shapes[i].contains_point(click)) {
                        shapes.erase(shapes.begin() + i);
                        if (selected_shape_index == i) selected_shape_index = -1;
                        else if (selected_shape_index > i) selected_shape_index--;
                        InvalidateRect(hwnd, NULL, FALSE);
                        break;
                    }
                }
            }
            return 0;
        }
        
        case WM_LBUTTONUP:
            dragging_light = false;
            dragging_shape = false;
            resizing_shape = false;
            ReleaseCapture();
            return 0;
            
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            
            if (resizing_shape && selected_shape_index >= 0) {
                Shape& shape = shapes[selected_shape_index];
                double dx = x - shape.center.x;
                double dy = y - shape.center.y;
                
                // figuring out the new size based on mouse position
                double new_size = std::max(20.0, std::sqrt(dx * dx + dy * dy));
                
                // making sure we don't overlap with anything
                bool can_resize = true;
                for (int i = 0; i < shapes.size(); i++) {
                    if (i == selected_shape_index) continue;
                    
                    double dx_center = shape.center.x - shapes[i].center.x;
                    double dy_center = shape.center.y - shapes[i].center.y;
                    double dist = std::sqrt(dx_center * dx_center + dy_center * dy_center);
                    
                    double other_size = shapes[i].is_light ? shapes[i].size1 : shapes[i].size1;
                    double min_distance = new_size + other_size;
                    
                    if (dist < min_distance) {
                        can_resize = false;
                        break;
                    }
                }
                
                if (can_resize) {
                    shape.size1 = new_size;
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (dragging_shape && selected_shape_index >= 0) {
                Point2D new_pos(x - drag_offset.x, y - drag_offset.y);
                
                // keeping shapes inside the canvas
                double margin = 50;
                if (new_pos.x - margin < 0) new_pos.x = margin;
                if (new_pos.x + margin > canvas_width) new_pos.x = canvas_width - margin;
                if (new_pos.y - margin < 0) new_pos.y = margin;
                if (new_pos.y + margin > canvas_height) new_pos.y = canvas_height - margin;
                
                // making sure we don't bump into other shapes
                bool can_move = true;
                for (int i = 0; i < shapes.size(); i++) {
                    if (i == selected_shape_index) continue;
                    
                    double dx = new_pos.x - shapes[i].center.x;
                    double dy = new_pos.y - shapes[i].center.y;
                    double dist = std::sqrt(dx * dx + dy * dy);
                    
                    double size_current = shapes[selected_shape_index].type == SHAPE_CIRCLE ? 
                                         shapes[selected_shape_index].size1 :
                                         std::max(shapes[selected_shape_index].size1, shapes[selected_shape_index].size2) * 0.7;
                    double size_other = shapes[i].is_light ? shapes[i].size1 :
                                       (shapes[i].type == SHAPE_CIRCLE ? shapes[i].size1 :
                                        std::max(shapes[i].size1, shapes[i].size2) * 0.7);
                    
                    if (dist < size_current + size_other) {
                        can_move = false;
                        break;
                    }
                }
                
                if (can_move) {
                    shapes[selected_shape_index].center = new_pos;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            else if (dragging_light) {
                Point2D new_pos(x, y);
                
                double lr = 30;
                for (auto& s : shapes) if (s.is_light) { lr = s.size1; break; }
                if (new_pos.x - lr < 0) new_pos.x = lr;
                if (new_pos.x + lr > canvas_width) new_pos.x = canvas_width - lr;
                if (new_pos.y - lr < 0) new_pos.y = lr;
                if (new_pos.y + lr > canvas_height) new_pos.y = canvas_height - lr;
                
                for (auto& s : shapes) {
                    if (s.is_light) continue;
                    double dx = new_pos.x - s.center.x, dy = new_pos.y - s.center.y;
                    double dist = std::sqrt(dx * dx + dy * dy);
                    double min_dist = lr + (s.type == SHAPE_CIRCLE ? s.size1 : std::max(s.size1, s.size2) * 0.7);
                    if (dist < min_dist) {
                        double angle = std::atan2(dy, dx);
                        new_pos.x = s.center.x + std::cos(angle) * min_dist;
                        new_pos.y = s.center.y + std::sin(angle) * min_dist;
                    }
                }
                
                light_pos = new_pos;
                for (auto& s : shapes) if (s.is_light) { s.center = light_pos; break; }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_SIZE: {
            canvas_width = LOWORD(lParam) - SIDEBAR_WIDTH;
            canvas_height = HIWORD(lParam);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT client_rect;
            GetClientRect(hwnd, &client_rect);
            int width = client_rect.right;
            int height = client_rect.bottom;
            
            // using double buffering to avoid flickering
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
            
            Render(memDC, width, height);
            
            BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
            
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const char CLASS_NAME[] = "Raytracing2D";
    
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassA(&wc);
    
    RECT rect = {0, 0, canvas_width + SIDEBAR_WIDTH, canvas_height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    HWND hwnd = CreateWindowExA(0, CLASS_NAME, "Raytracing 2D - Shape Editor",
        WS_OVERLAPPEDWINDOW, 100, 100, 
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL);
    
    if (!hwnd) return 0;
    
    // starting with a light and one circle
    shapes.push_back(Shape(SHAPE_CIRCLE, light_pos, 30, 0, true));
    shapes.push_back(Shape(SHAPE_CIRCLE, Point2D(500, 200), 70));
    
    ShowWindow(hwnd, nCmdShow);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
