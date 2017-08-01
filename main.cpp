#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFontMetrics>
#include <QOpenGLTexture>
#include <QPainter>
#include <QKeyEvent>
#include <QtGui/QGuiApplication>
//#include <QApplication>

#include <QtCore/qmath.h>
#include <iostream>
#include <sstream>

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
    #include "luajit.h"
}

#include "main.h"

lua_State *L;

void POBWindow::initializeGL() {
    std::cout << "initgl" << std::endl;
    QImage wimg(1, 1, QImage::Format_Mono);
    wimg.fill(1);
    white = new QOpenGLTexture(wimg);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void POBWindow::resizeGL(int w, int h) {
}

void POBWindow::paintGL() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    layers.clear();
    curLayer = 0;
    curSubLayer = 0;
    glColor4f(0, 0, 0, 0);
    painter = new QPainter(this);
    lua_getfield(L, LUA_REGISTRYINDEX, "uicallbacks");
    lua_getfield(L, -1, "MainObject");
    lua_getfield(L, -1, "OnFrame");
    lua_insert(L, -2);
    int result = lua_pcall(L, 1, 0, 0);
    if (result != 0) {
        lua_error(L);
    }
    painter->beginNativePainting();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (auto layer : layers) {
        for (auto cmd : layer) {
            cmd->execute();
        }
    }
    painter->endNativePainting();
    delete painter;
    painter = NULL;
    update();
}

void POBWindow::mouseMoveEvent(QMouseEvent *event) {
}

void pushMouseString(QMouseEvent *event) {
    switch (event->button()) {
    case Qt::LeftButton:
        lua_pushstring(L, "LEFTBUTTON");
        break;
    case Qt::RightButton:
        lua_pushstring(L, "RIGHTBUTTON");
        break;
    case Qt::MidButton:
        lua_pushstring(L, "MIDDLEBUTTON");
        break;
    default:
        std::cout << "MOUSE STRING? " << event->button() << std::endl;
    }
}

void POBWindow::mousePressEvent(QMouseEvent *event) {
    lua_getfield(L, LUA_REGISTRYINDEX, "uicallbacks");
    lua_getfield(L, -1, "MainObject");
    lua_getfield(L, -1, "OnKeyDown");
    lua_insert(L, -2);
    pushMouseString(event);
    lua_pushboolean(L, false);
    int result = lua_pcall(L, 3, 0, 0);
    if (result != 0) {
        lua_error(L);
    }
}

void POBWindow::mouseReleaseEvent(QMouseEvent *event) {
    lua_getfield(L, LUA_REGISTRYINDEX, "uicallbacks");
    lua_getfield(L, -1, "MainObject");
    lua_getfield(L, -1, "OnKeyUp");
    lua_insert(L, -2);
    pushMouseString(event);
    int result = lua_pcall(L, 2, 0, 0);
    if (result != 0) {
        lua_error(L);
    }
}

void POBWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    lua_getfield(L, LUA_REGISTRYINDEX, "uicallbacks");
    lua_getfield(L, -1, "MainObject");
    lua_getfield(L, -1, "OnKeyDown");
    lua_insert(L, -2);
    pushMouseString(event);
    lua_pushboolean(L, true);
    int result = lua_pcall(L, 3, 0, 0);
    if (result != 0) {
        lua_error(L);
    }
}

void POBWindow::wheelEvent(QWheelEvent *event) {
    lua_getfield(L, LUA_REGISTRYINDEX, "uicallbacks");
    lua_getfield(L, -1, "MainObject");
    lua_getfield(L, -1, "OnKeyUp");
    lua_insert(L, -2);
    if (event->angleDelta().y() > 0) {
        lua_pushstring(L, "WHEELUP");
    } else if (event->angleDelta().y() < 0) {
        lua_pushstring(L, "WHEELDOWN");
    } else {
        return;
    }
    lua_pushboolean(L, false);
    int result = lua_pcall(L, 3, 0, 0);
    if (result != 0) {
        lua_error(L);
    }
}

void POBWindow::keyPressEvent(QKeyEvent *event) {
    lua_getfield(L, LUA_REGISTRYINDEX, "uicallbacks");
    lua_getfield(L, -1, "MainObject");
    lua_getfield(L, -1, "OnKeyDown");
    switch (event->key()) {
    case Qt::Key_Escape:
        lua_pushstring(L, "ESCAPE");
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        lua_pushstring(L, "RETURN");
        break;
    case Qt::Key_Backspace:
        lua_pushstring(L, "BACK");
        break;
    default:
        if (event->key() >= ' ' && event->key() <= '~') {
            char s[2];
            if (event->key() >= 'A' && event->key() <= 'Z' && !(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)) {
                s[0] = event->key() + 32;
            } else {
                s[0] = event->key();
            }
            s[1] = 0;
            lua_pop(L, 1);
            lua_getfield(L, -1, "OnChar");
            lua_pushstring(L, s);

        } else {
            lua_pushstring(L, "ASDF");
            std::cout << "UNHANDLED KEYDOWN" << std::endl;
        }
    }
    lua_insert(L, -2);
    lua_insert(L, -3);
    lua_pushboolean(L, false);
    int result = lua_pcall(L, 3, 0, 0);
    if (result != 0) {
        lua_error(L);
    }
}

void POBWindow::keyReleaseEvent(QKeyEvent *event) {
    lua_getfield(L, LUA_REGISTRYINDEX, "uicallbacks");
    lua_getfield(L, -1, "MainObject");
    lua_getfield(L, -1, "OnKeyUp");
    lua_insert(L, -2);
    switch (event->key()) {
    case Qt::Key_Escape:
        lua_pushstring(L, "ESCAPE");
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        lua_pushstring(L, "RETURN");
        break;
    case Qt::Key_Backspace:
        lua_pushstring(L, "BACK");
        break;
    default:
        lua_pushstring(L, "ASDF");
        std::cout << "UNHANDLED KEYUP" << std::endl;
    }
    int result = lua_pcall(L, 2, 0, 0);
    if (result != 0) {
        lua_error(L);
    }
}

void POBWindow::LAssert(lua_State* L, int cond, const char* fmt, ...) {
    if ( !cond ) {
        va_list va;
        va_start(va, fmt);
        lua_pushvfstring(L, fmt, va);
        va_end(va);
        lua_error(L);
    }
}

int POBWindow::IsUserData(lua_State* L, int index, const char* metaName)
{
    if (lua_type(L, index) != LUA_TUSERDATA || lua_getmetatable(L, index) == 0) {
        return 0;
    }
    lua_getfield(L, LUA_REGISTRYINDEX, metaName);
    int ret = lua_rawequal(L, -2, -1);
    lua_pop(L, 2);
    return ret;
}

void POBWindow::SetDrawLayer(int layer) {
    SetDrawLayer(layer, 0);
}

void POBWindow::SetDrawLayer(int layer, int subLayer) {
    if (layer == curLayer && subLayer == curSubLayer) {
        return;
    }

    curLayer = layer;
    curSubLayer = subLayer;
    QPair<int, int> key = qMakePair(layer, subLayer);
    if (layers.contains(key)) {
        return;
    }
    layers[key] = QList<std::shared_ptr<Cmd>>();
}


void POBWindow::AppendCmd(std::shared_ptr<Cmd> cmd) {
    layers[qMakePair(curLayer, curSubLayer)].append(cmd);
}

void POBWindow::DrawColor(const float col[4]) {
    if (col) {
        drawColor[0] = col[0];
        drawColor[1] = col[1];
        drawColor[2] = col[2];
        drawColor[3] = col[3];
    } else {
        drawColor[0] = 1.0f;
        drawColor[1] = 1.0f;
        drawColor[2] = 1.0f;
        drawColor[3] = 1.0f;
    }
    AppendCmd(std::shared_ptr<Cmd>(new ColorCmd(drawColor)));
    /*QColor qcol(drawColor[0] * 255, drawColor[1] * 255, drawColor[2] * 255, drawColor[3] * 255);
    painter->setPen(qcol);
    painter->setBrush(qcol);*/
}

void POBWindow::DrawColor(uint32_t col) {
    drawColor[0] = ((col >> 16) & 0xFF) / 255.0f;
    drawColor[1] = ((col >> 8) & 0xFF) / 255.0f;
    drawColor[2] = (col & 0xFF) / 255.0f;
    drawColor[3] = (col >> 24) / 255.0f;
}


POBWindow *pobwindow;

// Color escape table
static const float colorEscape[10][4] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    {0.7f, 0.7f, 0.7f, 1.0f},
    {0.4f, 0.4f, 0.4f, 1.0f}
};

int IsColorEscape(const char* str)
{
    if (str[0] != '^') {
        return 0;
    }
    if (isdigit(str[1])) {
        return 2;
    } else if (str[1] == 'x' || str[1] == 'X') {
        for (int c = 0; c < 6; c++) {
            if ( !isxdigit(str[c + 2]) ) {
                return 0;
            }
        }
        return 8;
    }
    return 0;
}

void ReadColorEscape(const char* str, float* out)
{
    int len = IsColorEscape(str);
    switch (len) {
    case 2:
        out[0] = colorEscape[str[1] - '0'][0];
        out[1] = colorEscape[str[1] - '0'][1];
        out[2] = colorEscape[str[1] - '0'][2];
        break;
    case 8:
    {
        int xr, xg, xb;
        sscanf(str + 2, "%2x%2x%2x", &xr, &xg, &xb);
        out[0] = xr / 255.0f;
        out[1] = xg / 255.0f;
        out[2] = xb / 255.0f;
    }
    break;
    }
}

// =========
// Callbacks
// =========

static int l_SetCallback(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: SetCallback(name[, func])");
    pobwindow->LAssert(L, lua_isstring(L, 1), "SetCallback() argument 1: expected string, got %t", 1);
    lua_pushvalue(L, 1);
    if (n >= 2) {
        pobwindow->LAssert(L, lua_isfunction(L, 2) || lua_isnil(L, 2), "SetCallback() argument 2: expected function or nil, got %t", 2);
        lua_pushvalue(L, 2);
    } else {
        lua_pushnil(L);
    }
    lua_settable(L, lua_upvalueindex(1));
    return 0;
}

static int l_GetCallback(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: GetCallback(name)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "GetCallback() argument 1: expected string, got %t", 1);
    lua_pushvalue(L, 1);
    lua_gettable(L, lua_upvalueindex(1));
    return 1;
}

static int l_SetMainObject(lua_State* L)
{
    int n = lua_gettop(L);
    lua_pushstring(L, "MainObject");
    if (n >= 1) {
        pobwindow->LAssert(L, lua_istable(L, 1) || lua_isnil(L, 1), "SetMainObject() argument 1: expected table or nil, got %t", 1);
        lua_pushvalue(L, 1);
    } else {
        lua_pushnil(L);
    }
    lua_settable(L, lua_upvalueindex(1));
    return 0;
}

// =============
// Image Handles
// =============

struct imgHandle_s {
    QOpenGLTexture* hnd;
    QImage* img;
};

static int l_NewImageHandle(lua_State* L)
{
    imgHandle_s* imgHandle = (imgHandle_s*)lua_newuserdata(L, sizeof(imgHandle_s));
    imgHandle->hnd = NULL;
    imgHandle->img = NULL;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L, -2);
    return 1;
}

static imgHandle_s* GetImgHandle(lua_State* L, const char* method, bool loaded)
{
    pobwindow->LAssert(L, pobwindow->IsUserData(L, 1, "uiimghandlemeta"), "imgHandle:%s() must be used on an image handle", method);
    imgHandle_s* imgHandle = (imgHandle_s*)lua_touserdata(L, 1);
    lua_remove(L, 1);
    if (loaded) {
        //pobwindow->LAssert(L, imgHandle->hnd != NULL, "imgHandle:%s(): image handle has no image loaded", method);
    }
    return imgHandle;
}

static int l_imgHandleGC(lua_State* L)
{
    imgHandle_s* imgHandle = GetImgHandle(L, "__gc", false);
    delete imgHandle->hnd;
    delete imgHandle->img;
    return 0;
}

static int l_imgHandleLoad(lua_State* L) 
{
    imgHandle_s* imgHandle = GetImgHandle(L, "Load", false);
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: imgHandle:Load(fileName[, flag1[, flag2...]])");
    pobwindow->LAssert(L, lua_isstring(L, 1), "imgHandle:Load() argument 1: expected string, got %t", 1);
    QString fileName = lua_tostring(L, 1);
    QString fullFileName;
    if (fileName.contains(':') || pobwindow->scriptWorkDir.isEmpty()) {
        fullFileName = fileName;
    } else {
        fullFileName = pobwindow->scriptWorkDir + "\\" + fileName;
    }
    delete imgHandle->hnd;
    delete imgHandle->img;
    int flags = TF_NOMIPMAP;
    for (int f = 2; f <= n; f++) {
        if ( !lua_isstring(L, f) ) {
            continue;
        }
        const char* flag = lua_tostring(L, f);
        if ( !strcmp(flag, "ASYNC") ) {
            flags|= TF_ASYNC;
        } else if ( !strcmp(flag, "CLAMP") ) {
            flags|= TF_CLAMP;
        } else if ( !strcmp(flag, "MIPMAP") ) {
            flags&= ~TF_NOMIPMAP;
        } else {
            pobwindow->LAssert(L, 0, "imgHandle:Load(): unrecognised flag '%s'", flag);
        }
    }
    imgHandle->img = new QImage();
    imgHandle->img->load(fullFileName);
    imgHandle->img->setText("fname", fullFileName);
    //imgHandle->hnd = new QOpenGLTexture(img);
    //pobwindow->renderer->RegisterShader(fullFileName, flags);
    return 0;
}

static int l_imgHandleUnload(lua_State* L)
{
    imgHandle_s* imgHandle = GetImgHandle(L, "Unload", false);
    delete imgHandle->hnd;
    imgHandle->hnd = NULL;
    delete imgHandle->img;
    imgHandle->img = NULL;
    return 0;
}

static int l_imgHandleIsValid(lua_State* L)
{
    imgHandle_s* imgHandle = GetImgHandle(L, "IsValid", false);
    lua_pushboolean(L, imgHandle->hnd != NULL);
    return 1;
}

static int l_imgHandleIsLoading(lua_State* L)
{
    imgHandle_s* imgHandle = GetImgHandle(L, "IsLoading", true);
//	int width, height;
//	pobwindow->renderer->GetShaderImageSize(imgHandle->hnd, width, height);
    lua_pushboolean(L, false);
    return 1;
}

static int l_imgHandleSetLoadingPriority(lua_State* L)
{
    imgHandle_s* imgHandle = GetImgHandle(L, "SetLoadingPriority", true);
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: imgHandle:SetLoadingPriority(pri)");
    pobwindow->LAssert(L, lua_isnumber(L, 1), "imgHandle:SetLoadingPriority() argument 1: expected number, got %t", 1);
    //pobwindow->renderer->SetShaderLoadingPriority(imgHandle->hnd, (int)lua_tointeger(L, 1));
    return 0;
}

static int l_imgHandleImageSize(lua_State* L)
{
    imgHandle_s* imgHandle = GetImgHandle(L, "ImageSize", true);
    QSize size(imgHandle->img->size());
    lua_pushinteger(L, size.width());
    lua_pushinteger(L, size.height());
    return 2;
}

// =========
// Rendering
// =========

static int l_GetScreenSize(lua_State* L)
{
    QRect win = painter->window();
    lua_pushinteger(L, win.width());
    lua_pushinteger(L, win.height());
    return 2;
}

static int l_SetClearColor(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 3, "Usage: SetClearColor(red, green, blue[, alpha])");
    float color[4];
    for (int i = 1; i <= 3; i++) {
        pobwindow->LAssert(L, lua_isnumber(L, i), "SetClearColor() argument %d: expected number, got %t", i, i);
        color[i-1] = (float)lua_tonumber(L, i);
    }
    if (n >= 4 && !lua_isnil(L, 4)) {
        pobwindow->LAssert(L, lua_isnumber(L, 4), "SetClearColor() argument 4: expected number or nil, got %t", 4);
        color[3] = (float)lua_tonumber(L, 4);
    } else {
        color[3] = 1.0;
    }
    glClearColor(color[0], color[1], color[2], color[3]);
    return 0;
}

static int l_SetDrawLayer(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: SetDrawLayer({layer|nil}[, subLayer])");
    pobwindow->LAssert(L, lua_isnumber(L, 1) || lua_isnil(L, 1), "SetDrawLayer() argument 1: expected number or nil, got %t", 1);
    if (n >= 2) {
        pobwindow->LAssert(L, lua_isnumber(L, 2), "SetDrawLayer() argument 2: expected number, got %t", 2);
    }
    if (lua_isnil(L, 1)) {
        pobwindow->LAssert(L, n >= 2, "SetDrawLayer(): must provide subLayer if layer is nil");
        pobwindow->SetDrawSubLayer(lua_tointeger(L, 2));
    } else if (n >= 2) {
        pobwindow->SetDrawLayer(lua_tointeger(L, 1), lua_tointeger(L, 2));
    } else {
        pobwindow->SetDrawLayer(lua_tointeger(L, 1));
    }
    return 0;
}

static int l_SetViewport(lua_State* L)
{
    int n = lua_gettop(L);
    if (n) {
        pobwindow->LAssert(L, n >= 4, "Usage: SetViewport([x, y, width, height])");
        for (int i = 1; i <= 4; i++) {
            pobwindow->LAssert(L, lua_isnumber(L, i), "SetViewport() argument %d: expected number, got %t", i, i);
        }
        pobwindow->AppendCmd(std::shared_ptr<Cmd>(new ViewportCmd((int)lua_tointeger(L, 1), (int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4))));
    } else {
        QSize s(painter->window().size());
        pobwindow->AppendCmd(std::shared_ptr<Cmd>(new ViewportCmd(0, 0, s.width(), s.height())));
    }
    return 0;
}

static int l_SetDrawColor(lua_State* L)
{
    pobwindow->LAssert(L, painter != NULL, "SetDrawColor() called outside of OnFrame");
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: SetDrawColor(red, green, blue[, alpha]) or SetDrawColor(escapeStr)");
    float color[4];
    if (lua_type(L, 1) == LUA_TSTRING) {
        pobwindow->LAssert(L, IsColorEscape(lua_tostring(L, 1)), "SetDrawColor() argument 1: invalid color escape sequence");
        ReadColorEscape(lua_tostring(L, 1), color);
        color[3] = 1.0;
    } else {
        pobwindow->LAssert(L, n >= 3, "Usage: SetDrawColor(red, green, blue[, alpha]) or SetDrawColor(escapeStr)");
        for (int i = 1; i <= 3; i++) {
            pobwindow->LAssert(L, lua_isnumber(L, i), "SetDrawColor() argument %d: expected number, got %t", i, i);
            color[i-1] = (float)lua_tonumber(L, i);
        }
        if (n >= 4 && !lua_isnil(L, 4)) {
            pobwindow->LAssert(L, lua_isnumber(L, 4), "SetDrawColor() argument 4: expected number or nil, got %t", 4);
            color[3] = (float)lua_tonumber(L, 4);
        } else {
            color[3] = 1.0;
        }
    }
    pobwindow->DrawColor(color);
    return 0;
}

static int l_DrawImage(lua_State* L)
{
    pobwindow->LAssert(L, painter != NULL, "DrawImage() called outside of OnFrame");
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 5, "Usage: DrawImage({imgHandle|nil}, left, top, width, height[, tcLeft, tcTop, tcRight, tcBottom])");
    pobwindow->LAssert(L, lua_isnil(L, 1) || pobwindow->IsUserData(L, 1, "uiimghandlemeta"), "DrawImage() argument 1: expected image handle or nil, got %t", 1);
    QOpenGLTexture* hnd = NULL;
    if ( !lua_isnil(L, 1) ) {
        imgHandle_s* imgHandle = (imgHandle_s*)lua_touserdata(L, 1);
        if (imgHandle->hnd == NULL) {
            imgHandle->hnd = new QOpenGLTexture(*(imgHandle->img));
            if (!imgHandle->hnd->isCreated()) {
                std::cout << "SHITTY TEXTURE " << imgHandle->img->text("fname").toStdString() << std::endl;
                delete imgHandle->hnd;
                imgHandle->hnd = white;
            }
        }
        pobwindow->LAssert(L, imgHandle->hnd != NULL, "DrawImage(): image handle has no image loaded");
        hnd = imgHandle->hnd;
    }
    float arg[8];
    if (n > 5) {
        pobwindow->LAssert(L, n >= 9, "DrawImage(): incomplete set of texture coordinates provided");
        for (int i = 2; i <= 9; i++) {
            pobwindow->LAssert(L, lua_isnumber(L, i), "DrawImage() argument %d: expected number, got %t", i, i);
            arg[i-2] = (float)lua_tonumber(L, i);
        }
        pobwindow->AppendCmd(std::shared_ptr<Cmd>(new DrawImageCmd(hnd, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7])));
    } else {
        for (int i = 2; i <= 5; i++) {
            pobwindow->LAssert(L, lua_isnumber(L, i), "DrawImage() argument %d: expected number, got %t", i, i);
            arg[i-2] = (float)lua_tonumber(L, i);
        }
        pobwindow->AppendCmd(std::shared_ptr<Cmd>(new DrawImageCmd(hnd, arg[0], arg[1], arg[2], arg[3])));
    }
    return 0;
}

static int l_DrawImageQuad(lua_State* L)
{
    pobwindow->LAssert(L, painter != NULL, "DrawImageQuad() called outside of OnFrame");
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 9, "Usage: DrawImageQuad({imgHandle|nil}, x1, y1, x2, y2, x3, y3, x4, y4[, s1, t1, s2, t2, s3, t3, s4, t4])");
    pobwindow->LAssert(L, lua_isnil(L, 1) || pobwindow->IsUserData(L, 1, "uiimghandlemeta"), "DrawImageQuad() argument 1: expected image handle or nil, got %t", 1);
    QOpenGLTexture* hnd = NULL;
    if ( !lua_isnil(L, 1) ) {
        imgHandle_s* imgHandle = (imgHandle_s*)lua_touserdata(L, 1);
        if (imgHandle->hnd == NULL) {
            imgHandle->hnd = new QOpenGLTexture(*(imgHandle->img));
            if (!imgHandle->hnd->isCreated()) {
                std::cout << "SHITTY TEXTURE" << imgHandle->img->text("fname").toStdString() << std::endl;
                delete imgHandle->hnd;
                imgHandle->hnd = white;
            }
        }
        pobwindow->LAssert(L, imgHandle->hnd != NULL, "DrawImageQuad(): image handle has no image loaded");
        hnd = imgHandle->hnd;
    }
    float arg[16];
    if (n > 9) {
        pobwindow->LAssert(L, n >= 17, "DrawImageQuad(): incomplete set of texture coordinates provided");
        for (int i = 2; i <= 17; i++) {
            pobwindow->LAssert(L, lua_isnumber(L, i), "DrawImageQuad() argument %d: expected number, got %t", i, i);
            arg[i-2] = (float)lua_tonumber(L, i);
        }
        pobwindow->AppendCmd(std::shared_ptr<Cmd>(new DrawImageQuadCmd(hnd, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14], arg[15])));
    } else {
        for (int i = 2; i <= 9; i++) {
            pobwindow->LAssert(L, lua_isnumber(L, i), "DrawImageQuad() argument %d: expected number, got %t", i, i);
            arg[i-2] = (float)lua_tonumber(L, i);
        }
        pobwindow->AppendCmd(std::shared_ptr<Cmd>(new DrawImageQuadCmd(hnd, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7])));
    }
    return 0;
}

static int l_DrawString(lua_State* L)
{
    pobwindow->LAssert(L, painter != NULL, "DrawString() called outside of OnFrame");
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 6, "Usage: DrawString(left, top, align, height, font, text)");
    pobwindow->LAssert(L, lua_isnumber(L, 1), "DrawString() argument 1: expected number, got %t", 1);
    pobwindow->LAssert(L, lua_isnumber(L, 2), "DrawString() argument 2: expected number, got %t", 2);
    pobwindow->LAssert(L, lua_isstring(L, 3) || lua_isnil(L, 3), "DrawString() argument 3: expected string or nil, got %t", 3);
    pobwindow->LAssert(L, lua_isnumber(L, 4), "DrawString() argument 4: expected number, got %t", 4);
    pobwindow->LAssert(L, lua_isstring(L, 5), "DrawString() argument 5: expected string, got %t", 5);
    pobwindow->LAssert(L, lua_isstring(L, 6), "DrawString() argument 6: expected string, got %t", 6);
    static const char* alignMap[6] = { "LEFT", "CENTER", "RIGHT", "CENTER_X", "RIGHT_X", NULL };
    static const char* fontMap[4] = { "FIXED", "VAR", "VAR BOLD", NULL };
    //std::cout << "Drawstring: " << lua_tostring(L, 6) << " | align: " << lua_tostring(L, 3) << std::endl;
    pobwindow->AppendCmd(std::shared_ptr<Cmd>(new DrawStringCmd(
        (float)lua_tonumber(L, 1), (float)lua_tonumber(L, 2), luaL_checkoption(L, 3, "LEFT", alignMap), 
        (int)lua_tointeger(L, 4), luaL_checkoption(L, 5, "FIXED", fontMap), lua_tostring(L, 6)
                                                  )));
    return 0;
}

static int l_DrawStringWidth(lua_State* L) 
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 3, "Usage: DrawStringWidth(height, font, text)");
    pobwindow->LAssert(L, lua_isnumber(L, 1), "DrawStringWidth() argument 1: expected number, got %t", 1);
    pobwindow->LAssert(L, lua_isstring(L, 2), "DrawStringWidth() argument 2: expected string, got %t", 2);
    pobwindow->LAssert(L, lua_isstring(L, 3), "DrawStringWidth() argument 3: expected string, got %t", 3);
    static const char* fontMap[4] = { "FIXED", "VAR", "VAR BOLD", NULL };
    int fontsize = lua_tointeger(L, 1) - 6;
    QString fontname = lua_tostring(L, 2);
    QString text(lua_tostring(L, 3));

    text.remove(QRegExp("\\^x.{6}"));
    text.remove(QRegExp("\\^."));

    QFont font(fontname, fontsize);
    QFontMetrics fm(font);
    lua_pushinteger(L, fm.width(text));
    return 1;
//    lua_pushinteger(L, pobwindow->DrawStringWidth((int)lua_tointeger(L, 1), luaL_checkoption(L, 2, "FIXED", fontMap), lua_tostring(L, 3)));
    return 1;
}

static int l_DrawStringCursorIndex(lua_State* L) 
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 5, "Usage: DrawStringCursorIndex(height, font, text, cursorX, cursorY)");
    pobwindow->LAssert(L, lua_isnumber(L, 1), "DrawStringCursorIndex() argument 1: expected number, got %t", 1);
    pobwindow->LAssert(L, lua_isstring(L, 2), "DrawStringCursorIndex() argument 2: expected string, got %t", 2);
    pobwindow->LAssert(L, lua_isstring(L, 3), "DrawStringCursorIndex() argument 3: expected string, got %t", 3);
    pobwindow->LAssert(L, lua_isnumber(L, 4), "DrawStringCursorIndex() argument 4: expected number, got %t", 4);
    pobwindow->LAssert(L, lua_isnumber(L, 5), "DrawStringCursorIndex() argument 5: expected number, got %t", 5);
    static const char* fontMap[4] = { "FIXED", "VAR", "VAR BOLD", NULL };
//    lua_pushinteger(L, pobwindow->DrawStringCursorIndex((int)lua_tointeger(L, 1), luaL_checkoption(L, 2, "FIXED", fontMap), lua_tostring(L, 3), (int)lua_tointeger(L, 4), (int)lua_tointeger(L, 5)) + 1);
    lua_pushinteger(L, 5);
    return 1;
}

static int l_StripEscapes(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: StripEscapes(string)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "StripEscapes() argument 1: expected string, got %t", 1);
    const char* str = lua_tostring(L, 1);
    char* strip = new char[strlen(str) + 1];
    char* p = strip;
    while (*str) {
        int esclen = IsColorEscape(str);
        if (esclen) {
            str+= esclen;
        } else {
            *(p++) = *(str++);
        }
    }
    *p = 0;
    lua_pushstring(L, strip);
    delete strip;
    return 1;
}

static int l_GetAsyncCount(lua_State* L)
{
//    lua_pushinteger(L, pobwindow->GetTexAsyncCount());
    lua_pushinteger(L, 0);
    return 1;
}

// ==============
// Search Handles
// ==============

struct searchHandle_s {
    QFileInfoList *fil;
};

static int l_NewFileSearch(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: NewFileSearch(spec[, findDirectories])");
    pobwindow->LAssert(L, lua_isstring(L, 1), "NewFileSearch() argument 1: expected string, got %t", 1);
    QString search_string(lua_tostring(L, 1));
    QStringList split = search_string.split("/");
    QString wildcard = split.takeLast();
    QDir dir(split.join("/"));
    QStringList filters;
    filters << wildcard;
    bool dirOnly = lua_toboolean(L, 2) != 0;
    QFileInfoList fil = dir.entryInfoList(filters, QDir::NoDotAndDotDot | (dirOnly ? QDir::Dirs : QDir::Files));
    if (fil.isEmpty()) {
        return 0;
    }

    searchHandle_s *handle = (searchHandle_s*)lua_newuserdata(L, sizeof(searchHandle_s));
    handle->fil = new QFileInfoList(fil);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L, -2);
    return 1;
}

static QFileInfoList* GetSearchHandle(lua_State* L, const char* method, bool valid)
{
    pobwindow->LAssert(L, pobwindow->IsUserData(L, 1, "uisearchhandlemeta"), "searchHandle:%s() must be used on a search handle", method);
    searchHandle_s *searchHandle = (searchHandle_s*)lua_touserdata(L, 1);
    lua_remove(L, 1);
    if (valid) {
        pobwindow->LAssert(L, !searchHandle->fil->isEmpty(), "searchHandle:%s(): search handle is no longer valid (ran out of files to find)", method);
    }
    return searchHandle->fil;
}

static int l_searchHandleGC(lua_State* L)
{
    QFileInfoList* searchHandle = GetSearchHandle(L, "__gc", false);
    delete searchHandle;
    return 0;
}

static int l_searchHandleNextFile(lua_State* L)
{
    QFileInfoList* searchHandle = GetSearchHandle(L, "NextFile", true);
    searchHandle->removeFirst();
    if ( searchHandle->isEmpty() ) {
        return 0;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int l_searchHandleGetFileName(lua_State* L)
{
    QFileInfoList* searchHandle = GetSearchHandle(L, "GetFileName", true);
    lua_pushstring(L, searchHandle->first().fileName().toStdString().c_str());
    return 1;
}

static int l_searchHandleGetFileSize(lua_State* L)
{
    QFileInfoList* searchHandle = GetSearchHandle(L, "GetFileSize", true);
    lua_pushinteger(L, searchHandle->first().size());
    return 1;
}

static int l_searchHandleGetFileModifiedTime(lua_State* L)
{
    QFileInfoList* searchHandle = GetSearchHandle(L, "GetFileModifiedTime", true);
    QDateTime modified = searchHandle->first().lastModified();
    lua_pushnumber(L, modified.toMSecsSinceEpoch());
    lua_pushstring(L, modified.date().toString().toStdString().c_str());
    lua_pushstring(L, modified.time().toString().toStdString().c_str());
    return 3;
}

// =================
// General Functions
// =================

static int l_SetWindowTitle(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: SetWindowTitle(title)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "SetWindowTitle() argument 1: expected string, got %t", 1);
    pobwindow->setTitle(lua_tostring(L, 1));
    return 0;
}

static int l_GetCursorPos(lua_State* L)
{
    QPoint pos = QCursor::pos();
    pos = pobwindow->mapFromGlobal(pos);
    lua_pushinteger(L, pos.x());
    lua_pushinteger(L, pos.y());
    return 2;
}

static int l_SetCursorPos(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 2, "Usage: SetCursorPos(x, y)");
    pobwindow->LAssert(L, lua_isnumber(L, 1), "SetCursorPos() argument 1: expected number, got %t", 1);
    pobwindow->LAssert(L, lua_isnumber(L, 2), "SetCursorPos() argument 2: expected number, got %t", 2);
    //pobwindow->sys->video->SetRelativeCursor((int)lua_tointeger(L, 1), (int)lua_tointeger(L, 2));
    return 0;
}

static int l_ShowCursor(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: ShowCursor(doShow)");
    //pobwindow->sys->ShowCursor(lua_toboolean(L, 1));
    return 0;
}

static int l_IsKeyDown(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: IsKeyDown(keyName)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "IsKeyDown() argument 1: expected string, got %t", 1);
    size_t len;
    const char* kname = lua_tolstring(L, 1, &len);
    pobwindow->LAssert(L, len >= 1, "IsKeyDown() argument 1: string is empty", 1);
    QString k(kname);
    bool result = false;
    if (k == "LEFTBUTTON") {
        if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
            result = true;
        }
    } else {
        int keys = QGuiApplication::keyboardModifiers();
        if (k == "CTRL") {
            result =  keys & Qt::ControlModifier;
        } else if (k == "SHIFT") {
            result =  keys & Qt::ShiftModifier;
        } else if (k == "ALT") {
            result = keys & Qt::AltModifier;
        } else {
            std::cout << "UNKNOWN ISKEYDOWN: " << k.toStdString() << std::endl;
        }
    }
    lua_pushboolean(L, result);
    //int key = pobwindow->KeyForName(kname);
    //pobwindow->LAssert(L, key, "IsKeyDown(): unrecognised key name");
    //lua_pushboolean(L, pobwindow->sys->IsKeyDown(key));
    return 1;
}

static int l_Copy(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: Copy(string)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "Copy() argument 1: expected string, got %t", 1);
    //pobwindow->sys->ClipboardCopy(lua_tostring(L, 1));
    return 0;
}

static int l_Paste(lua_State* L)
{
    return 0;
    /*
    char* data = pobwindow->sys->ClipboardPaste();
    if (data) {
        lua_pushstring(L, data);
        FreeString(data);
        return 1;
    } else {
        return 0;
    }
    */
}

static int l_Deflate(lua_State* L)
{
    return 0;
/*	ui_main_c* ui = GetUIPtr(L);
	int n = lua_gettop(L);
	pobwindow->LAssert(L, n >= 1, "Usage: Deflate(string)");
	pobwindow->LAssert(L, lua_isstring(L, 1), "Deflate() argument 1: expected string, got %t", 1);
	z_stream_s z;
	z.zalloc = NULL;
	z.zfree = NULL;
	deflateInit(&z, 9);
	size_t inLen;
	byte* in = (byte*)lua_tolstring(L, 1, &inLen);
	int outSz = deflateBound(&z, inLen);
	byte* out = new byte[outSz];
	z.next_in = in;
	z.avail_in = inLen;
	z.next_out = out;
	z.avail_out = outSz;
	int err = deflate(&z, Z_FINISH);
	deflateEnd(&z);
	if (err == Z_STREAM_END) {
        lua_pushlstring(L, (const char*)out, z.total_out);
        return 1;
	} else {
        lua_pushnil(L);
        lua_pushstring(L, zError(err));
        return 2;
	}
*/
}

static int l_Inflate(lua_State* L)
{
    return 0;
    /*
        int n = lua_gettop(L);
      pobwindow->LAssert(L, n >= 1, "Usage: Inflate(string)");
      pobwindow->LAssert(L, lua_isstring(L, 1), "Inflate() argument 1: expected string, got %t", 1);
      size_t inLen;
      byte* in = (byte*)lua_tolstring(L, 1, &inLen);
      int outSz = inLen * 4;
      byte* out = new byte[outSz];
      z_stream_s z;
      z.next_in = in;
      z.avail_in = inLen;
      z.zalloc = NULL;
      z.zfree = NULL;
      z.next_out = out;
      z.avail_out = outSz;
      inflateInit(&z);
      int err;
      while ((err = inflate(&z, Z_NO_FLUSH)) == Z_OK) {
      if (z.avail_out == 0) {
      // Output buffer filled, embiggen it
      int newSz = outSz << 1;
      trealloc(out, newSz);
      z.next_out = out + outSz;
      z.avail_out = outSz;
      outSz = newSz;
      }
      }
      inflateEnd(&z);
      if (err == Z_STREAM_END) {
      lua_pushlstring(L, (const char*)out, z.total_out);
      return 1;
      } else {
      lua_pushnil(L);
      lua_pushstring(L, zError(err));
      return 2;
      }
    */
}

static int l_GetTime(lua_State* L)
{
    qint64 ms = QDateTime::currentDateTime().toMSecsSinceEpoch();
    lua_pushinteger(L, ms / 1000);
    return 1;
}

static int l_GetScriptPath(lua_State* L)
{
    lua_pushstring(L, "/home/phil/projects/pob/PathOfBuilding"); // FIXME
//    lua_pushstring(L, pobwindow->scriptPath);
    return 1;
}

static int l_GetRuntimePath(lua_State* L)
{
    lua_pushstring(L, "/home/phil/projects/pob/PathOfBuilding"); // FIXME
//    lua_pushstring(L, pobwindow->sys->basePath);
    return 1;
}

static int l_GetUserPath(lua_State* L)
{
    lua_pushstring(L, "/home/phil/projects/pob/PathOfBuilding"); // FIXME
//    lua_pushstring(L, pobwindow->sys->userPath);
    return 1;
}

static int l_MakeDir(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: MakeDir(path)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "MakeDir() argument 1: expected string, got %t", 1);
    lua_pushboolean(L, QDir().mkpath(lua_tostring(L, 1)));
    return 1;
}

static int l_RemoveDir(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: l_RemoveDir(path)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "l_RemoveDir() argument 1: expected string, got %t", 1);
    QDir d;
    if (!d.rmdir(lua_tostring(L, 1))) {
        lua_pushnil(L);
        return 1;
    } else {
        lua_pushboolean(L, true);
        return 1;
    }
}

static int l_SetWorkDir(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: SetWorkDir(path)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "SetWorkDir() argument 1: expected string, got %t", 1);
    return 0; // FIXME
    /*
    const char* newWorkDir = lua_tostring(L, 1);
    if ( !pobwindow->sys->SetWorkDir(newWorkDir) ) {
        if (pobwindow->scriptWorkDir) {
            FreeString(pobwindow->scriptWorkDir);
        }
        pobwindow->scriptWorkDir = AllocString(newWorkDir);
    }
    return 0;
    */
}

static int l_GetWorkDir(lua_State* L)
{
    lua_pushstring(L, "/home/phil/projects/pob/PathOfBuilding"); // FIXME
//    lua_pushstring(L, pobwindow->scriptWorkDir);
    return 1;
}

static int l_LaunchSubScript(lua_State* L)
{
    return 0;
    /*
        int n = lua_gettop(L);
      pobwindow->LAssert(L, n >= 3, "Usage: LaunchSubScript(scriptText, funcList, subList[, ...])");
      for (int i = 1; i <= 3; i++) {
      pobwindow->LAssert(L, lua_isstring(L, i), "LaunchSubScript() argument %d: expected string, got %t", i, i);
      }
      for (int i = 4; i <= n; i++) {
      pobwindow->LAssert(L, lua_isnil(L, i) || lua_isboolean(L, i) || lua_isnumber(L, i) || lua_isstring(L, i), 
      "LaunchSubScript() argument %d: only nil, boolean, number and string types can be passed to sub script", i);
      }
      notdword slot = -1;
      for (notdword i = 0; i < pobwindow->subScriptSize; i++) {
      if ( !pobwindow->subScriptList[i] ) {
      slot = i;
      break;
      }
      }
      if (slot == -1) {
      slot = pobwindow->subScriptSize;
      pobwindow->subScriptSize<<= 1;
      trealloc(pobwindow->subScriptList, pobwindow->subScriptSize);
      for (notdword i = slot; i < pobwindow->subScriptSize; i++) {
      pobwindow->subScriptList[i] = NULL;
      }
      }
      pobwindow->subScriptList[slot] = ui_ISubScript::GetHandle(ui, slot);
      if (pobwindow->subScriptList[slot]->Start()) {
      lua_pushlightuserdata(L, (void*)slot);
      } else {
      lua_pushnil(L);
      }
      return 1;
    */
}

static int l_AbortSubScript(lua_State* L)
{
    return 0;
    /*
        int n = lua_gettop(L);
      pobwindow->LAssert(L, n >= 1, "Usage: AbortSubScript(ssID)");
      pobwindow->LAssert(L, lua_islightuserdata(L, 1), "AbortSubScript() argument 1: expected subscript ID, got %t", 1);
      notdword slot = (notdword)lua_touserdata(L, 1);
      pobwindow->LAssert(L, slot < pobwindow->subScriptSize && pobwindow->subScriptList[slot], "AbortSubScript() argument 1: invalid subscript ID");
      pobwindow->LAssert(L, pobwindow->subScriptList[slot]->IsRunning(), "AbortSubScript(): subscript isn't running");
      ui_ISubScript::FreeHandle(pobwindow->subScriptList[slot]);
      pobwindow->subScriptList[slot] = NULL;
      return 0;
    */
}

static int l_IsSubScriptRunning(lua_State* L)
{
    return 0;
    /*
        int n = lua_gettop(L);
      pobwindow->LAssert(L, n >= 1, "Usage: IsSubScriptRunning(ssID)");
      pobwindow->LAssert(L, lua_islightuserdata(L, 1), "IsSubScriptRunning() argument 1: expected subscript ID, got %t", 1);
      notdword slot = (notdword)lua_touserdata(L, 1);
      pobwindow->LAssert(L, slot < pobwindow->subScriptSize && pobwindow->subScriptList[slot], "IsSubScriptRunning() argument 1: invalid subscript ID");
      lua_pushboolean(L, pobwindow->subScriptList[slot]->IsRunning());
      return 1;
    */
}

static int l_LoadModule(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: LoadModule(name[, ...])");
    pobwindow->LAssert(L, lua_isstring(L, 1), "LoadModule() argument 1: expected string, got %t", 1);
    QString fileName(lua_tostring(L, 1));
    if (!fileName.endsWith(".lua")) {
        fileName = fileName + ".lua";
    }
//    pobwindow->sys->SetWorkDir(pobwindow->scriptPath);
    int err = luaL_loadfile(L, fileName.toStdString().c_str());
//    pobwindow->sys->SetWorkDir(pobwindow->scriptWorkDir);
    pobwindow->LAssert(L, err == 0, "LoadModule() error loading '%s':\n%s", fileName.toStdString().c_str(), lua_tostring(L, -1));
    lua_replace(L, 1);	// Replace module name with module main chunk
    lua_call(L, n - 1, LUA_MULTRET);
    return lua_gettop(L);
}

static int l_PLoadModule(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: PLoadModule(name[, ...])");
    pobwindow->LAssert(L, lua_isstring(L, 1), "PLoadModule() argument 1: expected string, got %t", 1);
    QString fileName(lua_tostring(L, 1));
    if (!fileName.endsWith(".lua")) {
        fileName = fileName + ".lua";
    }
    //pobwindow->sys->SetWorkDir(pobwindow->scriptPath);
    int err = luaL_loadfile(L, fileName.toStdString().c_str());
    //pobwindow->sys->SetWorkDir(pobwindow->scriptWorkDir);
    if (err) {
        return 1;
    }
    lua_replace(L, 1);	// Replace module name with module main chunk
    //lua_getfield(L, LUA_REGISTRYINDEX, "traceback");
    //lua_insert(L, 1); // Insert traceback function at start of stack
    err = lua_pcall(L, n - 1, LUA_MULTRET, 0);
    if (err) {
        return 1;
    }
    lua_pushnil(L);
    lua_insert(L, 1);
//    lua_replace(L, 1); // Replace traceback function with nil
    return lua_gettop(L);
}

static int l_PCall(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: PCall(func[, ...])");
    pobwindow->LAssert(L, lua_isfunction(L, 1), "PCall() argument 1: expected function, got %t", 1);
//    lua_getfield(L, LUA_REGISTRYINDEX, "traceback");
//    lua_insert(L, 1); // Insert traceback function at start of stack
    int err = lua_pcall(L, n - 1, LUA_MULTRET, 0);
    if (err) {
        lua_error(L);
        return 1;
    }
//    lua_pushnil(L);
//    lua_replace(L, 1); // Replace traceback function with nil
    return lua_gettop(L);
}

static int l_ConPrintf(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: ConPrintf(fmt[, ...])");
    pobwindow->LAssert(L, lua_isstring(L, 1), "ConPrintf() argument 1: expected string, got %t", 1);
    lua_pushvalue(L, lua_upvalueindex(1));	// string.format
    lua_insert(L, 1);
    lua_call(L, n, 1);
    pobwindow->LAssert(L, lua_isstring(L, 1), "ConPrintf() error: string.format returned non-string");
    std::cout << lua_tostring(L, 1) << std::endl;
    //pobwindow->sys->con->Printf("%s\n", lua_tostring(L, 1));
    return 0;
}

static void printTableItter(lua_State* L, int index, int level, bool recurse)
{
    lua_checkstack(L, 5);
    lua_pushnil(L);
    while (lua_next(L, index)) {
        for (int t = 0; t < level; t++) std::cout << "  ";
        // Print key
        if (lua_type(L, -2) == LUA_TSTRING) {
            std::cout << "[\"" << lua_tostring(L, -2) << "\"] = ";
        } else {
            lua_pushvalue(L, 2);	// Push tostring function
            lua_pushvalue(L, -3);	// Push key
            lua_call(L, 1, 1);		// Call tostring
            std::cout << lua_tostring(L, -1) << " = ";
            lua_pop(L, 1);			// Pop result of tostring
        }
        // Print value
        if (lua_type(L, -1) == LUA_TTABLE) {
            bool expand = recurse;
            if (expand) {
                lua_pushvalue(L, -1);	// Push value
                lua_gettable(L, 3);		// Index printed tables list
                expand = lua_toboolean(L, -1) == 0;
                lua_pop(L, 1);			// Pop result of indexing
            }
            if (expand) {
                lua_pushvalue(L, -1);	// Push value
                lua_pushboolean(L, 1);
                lua_settable(L, 3);		// Add to printed tables list
                std::cout << "table: " << lua_topointer(L, -1) << " {"
                          << std::endl;
                printTableItter(L, lua_gettop(L), level + 1, true);
                for (int t = 0; t < level; t++) std::cout << "  ";
                std::cout << "}" << std::endl;
            } else {
                std::cout << "table: " << lua_topointer(L, -1) << " { ... }\n";
            }
        } else if (lua_type(L, -1) == LUA_TSTRING) {
            std::cout << "\"" << lua_tostring(L, -1) << "\"" << std::endl;
        } else {
            lua_pushvalue(L, 2);	// Push tostring function
            lua_pushvalue(L, -2);	// Push value
            lua_call(L, 1, 1);		// Call tostring
            std::cout << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);			// Pop result of tostring
        }
        lua_pop(L, 1);	// Pop value
    }
}

static int l_ConPrintTable(lua_State* L)
{
    return 0;
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: ConPrintTable(tbl[, noRecurse])");
    pobwindow->LAssert(L, lua_istable(L, 1), "ConPrintTable() argument 1: expected table, got %t", 1);
    bool recurse = lua_toboolean(L, 2) == 0;
    lua_settop(L, 1);
    lua_getglobal(L, "tostring");
    lua_newtable(L);		// Printed tables list
    lua_pushvalue(L, 1);	// Push root table
    lua_pushboolean(L, 1);
    lua_settable(L, 3);		// Add root table to printed tables list
    printTableItter(L, 1, 0, recurse);
    return 0;
}

static int l_ConExecute(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: ConExecute(cmd)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "ConExecute() argument 1: expected string, got %t", 1);
    //pobwindow->sys->con->Execute(lua_tostring(L,1)); // FIXME
    return 0;
}

static int l_ConClear(lua_State* L)
{
//    pobwindow->sys->con->Clear();
    return 0;
}

static int l_print(lua_State* L)
{
    int n = lua_gettop(L);
    lua_getglobal(L, "tostring");
    for (int i = 1; i <= n; i++) {
        lua_pushvalue(L, -1);	// Push tostring function
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);		// Call tostring
        const char* s = lua_tostring(L, -1);
        pobwindow->LAssert(L, s != NULL, "print() error: tostring returned non-string");
        if (i > 1) std::cout << " ";
        std::cout << s;
        lua_pop(L, 1);			// Pop result of tostring
    }
    std::cout << std::endl;
    return 0;
}

static int l_SpawnProcess(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: SpawnProcess(cmdName[, args])");
    pobwindow->LAssert(L, lua_isstring(L, 1), "SpawnProcess() argument 1: expected string, got %t", 1);
    // FIXME
//    pobwindow->sys->SpawnProcess(lua_tostring(L, 1), lua_tostring(L, 2));
    return 0;
}

static int l_OpenURL(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: OpenURL(url)");
    pobwindow->LAssert(L, lua_isstring(L, 1), "OpenURL() argument 1: expected string, got %t", 1);
    // FIXME
    //pobwindow->sys->OpenURL(lua_tostring(L, 1));
    return 0;
}

static int l_SetProfiling(lua_State* L)
{
    int n = lua_gettop(L);
    pobwindow->LAssert(L, n >= 1, "Usage: SetProfiling(isEnabled)");
    // FIXME
    //pobwindow->debug->SetProfiling(lua_toboolean(L, 1) == 1);
    return 0;
}

static int l_Restart(lua_State* L)
{
    // FIXME
    //pobwindow->restartFlag = true;
    return 0;
}

static int l_Exit(lua_State* L)
{
    int n = lua_gettop(L);
    const char* msg = NULL;
    if (n >= 1 && !lua_isnil(L, 1)) {
        pobwindow->LAssert(L, lua_isstring(L, 1), "Exit() argument 1: expected string or nil, got %t", 1);
        msg = lua_tostring(L, 1);
    }
    // FIXME
    //pobwindow->sys->Exit(msg);
    //pobwindow->didExit = true;
//	lua_pushstring(L, "dummy");
//	lua_error(L);
    return 0;
}

#define ADDFUNC(n) lua_pushcclosure(L, l_##n, 0);lua_setglobal(L, #n);
#define ADDFUNCCL(n, u) lua_pushcclosure(L, l_##n, u);lua_setglobal(L, #n);

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    pobwindow = new POBWindow;

    std::cout << "settin up" << std::endl;
    L = luaL_newstate();
    luaL_openlibs(L);
    //std::cout << "JIT DISABLE: " << luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_OFF) << std::endl;

    std::cout << "stated" << std::endl;
    // Callbacks
    lua_newtable(L);		// Callbacks table
    lua_pushvalue(L, -1);	// Push callbacks table
    ADDFUNCCL(SetCallback, 1);
    lua_pushvalue(L, -1);	// Push callbacks table
    ADDFUNCCL(GetCallback, 1);
    lua_pushvalue(L, -1);	// Push callbacks table
    ADDFUNCCL(SetMainObject, 1);
    lua_setfield(L, LUA_REGISTRYINDEX, "uicallbacks");

    // Image handles
    lua_newtable(L);		// Image handle metatable
    lua_pushvalue(L, -1);	// Push image handle metatable
    ADDFUNCCL(NewImageHandle, 1);
    lua_pushvalue(L, -1);	// Push image handle metatable
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_imgHandleGC);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, l_imgHandleLoad);
    lua_setfield(L, -2, "Load");
    lua_pushcfunction(L, l_imgHandleUnload);
    lua_setfield(L, -2, "Unload");
    lua_pushcfunction(L, l_imgHandleIsValid);
    lua_setfield(L, -2, "IsValid");
    lua_pushcfunction(L, l_imgHandleIsLoading);
    lua_setfield(L, -2, "IsLoading");
    lua_pushcfunction(L, l_imgHandleSetLoadingPriority);
    lua_setfield(L, -2, "SetLoadingPriority");
    lua_pushcfunction(L, l_imgHandleImageSize);
    lua_setfield(L, -2, "ImageSize");
    lua_setfield(L, LUA_REGISTRYINDEX, "uiimghandlemeta");

    // Rendering
    ADDFUNC(GetScreenSize);
    ADDFUNC(SetClearColor);
    ADDFUNC(SetDrawLayer);
    ADDFUNC(SetViewport);
    ADDFUNC(SetDrawColor);
    ADDFUNC(DrawImage);
    ADDFUNC(DrawImageQuad);
    ADDFUNC(DrawString);
    ADDFUNC(DrawStringWidth);
    ADDFUNC(DrawStringCursorIndex);
    ADDFUNC(StripEscapes);
    ADDFUNC(GetAsyncCount);

    // Search handles
    lua_newtable(L);	// Search handle metatable
    lua_pushvalue(L, -1);	// Push search handle metatable
    ADDFUNCCL(NewFileSearch, 1);
    lua_pushvalue(L, -1);	// Push search handle metatable
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_searchHandleGC);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, l_searchHandleNextFile);
    lua_setfield(L, -2, "NextFile");
    lua_pushcfunction(L, l_searchHandleGetFileName);
    lua_setfield(L, -2, "GetFileName");
    lua_pushcfunction(L, l_searchHandleGetFileSize);
    lua_setfield(L, -2, "GetFileSize");
    lua_pushcfunction(L, l_searchHandleGetFileModifiedTime);
    lua_setfield(L, -2, "GetFileModifiedTime");
    lua_setfield(L, LUA_REGISTRYINDEX, "uisearchhandlemeta");

    // General function
    ADDFUNC(SetWindowTitle);
    ADDFUNC(GetCursorPos);
    ADDFUNC(SetCursorPos);
    ADDFUNC(ShowCursor);
    ADDFUNC(IsKeyDown);
    ADDFUNC(Copy);
    ADDFUNC(Paste);
    ADDFUNC(Deflate);
    ADDFUNC(Inflate);
    ADDFUNC(GetTime);
    ADDFUNC(GetScriptPath);
    ADDFUNC(GetRuntimePath);
    ADDFUNC(GetUserPath);
    ADDFUNC(MakeDir);
    ADDFUNC(RemoveDir);
    ADDFUNC(SetWorkDir);
    ADDFUNC(GetWorkDir);
    ADDFUNC(LaunchSubScript);
    ADDFUNC(AbortSubScript);
    ADDFUNC(IsSubScriptRunning);
    ADDFUNC(LoadModule);
    ADDFUNC(PLoadModule);
    ADDFUNC(PCall);
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "format");
    ADDFUNCCL(ConPrintf, 1);
    lua_pop(L, 1);		// Pop 'string' table
    ADDFUNC(ConPrintTable);
    ADDFUNC(ConExecute);
    ADDFUNC(ConClear);
    ADDFUNC(print);
    ADDFUNC(SpawnProcess);
    ADDFUNC(OpenURL);
    ADDFUNC(SetProfiling);
    ADDFUNC(Restart);
    ADDFUNC(Exit);
    lua_getglobal(L, "os");
    lua_pushcfunction(L, l_Exit);
    lua_setfield(L, -2, "exit");
    lua_pop(L, 1);		// Pop 'os' table

    int result = luaL_dofile(L, "../PathOfBuilding/Launch.lua");
    if (result != 0) {
        lua_error(L);
    }

    lua_getfield(L, LUA_REGISTRYINDEX, "uicallbacks");
    lua_getfield(L, -1, "MainObject");
    lua_getfield(L, -1, "OnInit");
    lua_insert(L, -2);
    result = lua_pcall(L, 1, 0, 0);
    if (result != 0) {
        lua_error(L);
    }
    pobwindow->show();
    return app.exec();
}

