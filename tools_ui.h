/*
 * tools_ui.h
 * Author: Jesus Diaz Garcia
 *
 * Immediate mode UI API.
 */

#ifndef TOOLS_UI_H
#define TOOLS_UI_H

#ifndef TOOLS_GFX_H
#error "tools_gfx.h needs to be defined before tools_ui.h"
#endif

#define UI_TEMP_STRING_SIZE KB(4)

#define UI_VSPRINTF(format, text) \
	va_list vaList; \
	va_start(vaList, format); \
	VSNPrintf(ui.tempString, UI_TEMP_STRING_SIZE, format, vaList); \
	va_end(vaList); \
	const char *text = ui.tempString;

// Colors
constexpr float4 UiColorWhite = { 1.0, 1.0, 1.0, 1.0 };
constexpr float4 UiColorOrange = {1.0, 0.6, 0.0, 1.0};
constexpr float4 UiColorBorder = { 0.2, 0.2, 0.2, 0.9 };
constexpr float4 UiColorCaption = { 0.1, 0.2, 0.4, 1.0 };
constexpr float4 UiColorCaptionInactive = { 0.1, 0.1, 0.1, 1.0 };
constexpr float4 UiColorPanel = { 0.05, 0.05, 0.05, 0.95 };
constexpr float4 UiColorWidget = { 0.1, 0.2, 0.4, 1.0 };
constexpr float4 UiColorWidgetHover = { 0.2, 0.4, 1.0, 1.0 };
constexpr float4 UiColorWidgetInactive = { 0.02, 0.05, 0.1, 1.0 };
constexpr float4 UiColorBox = { 0.05, 0.1, 0.2, 1.0 };
constexpr float4 UiColorBoxHover = { 0.1, 0.2, 0.4, 1.0 };
constexpr float4 UiColorScrollbar = { 0.2, 0.2, 0.2, 1.0 };
constexpr float4 UiColorScrollbarHover = { 0.4, 0.4, 0.4, 1.0 };
constexpr float4 UiColorBoxInactive = { 0.01, 0.02, 0.05, 1.0 };
constexpr float4 UiColorMenu = { 0.05, 0.05, 0.05, 1.0 };
constexpr float4 UiColorMenuHover = { 0.1, 0.1, 0.1, 1.0 };

// Metrics
constexpr float2 UiBorderSize = { 1.0, 1.0 };
constexpr float2 UiWindowPadding = { 4.0f, 8.0f };
constexpr float UiSpacing = 8.0f;

struct UIVertex
{
	float2 position;
	float2 texCoord;
	rgba color;
};

struct UIInput
{
	Mouse mouse;
	Keyboard keyboard;
	Chars chars;
	int2 lastMouseClickPos;
};

struct UISection
{
	u32 hash;
	bool open;
};

enum UIWindowFlags
{
	UIWindowFlag_None = 0,
	UIWindowFlag_Resizable = 1 << 0,
	UIWindowFlag_Titlebar = 1 << 1,
	UIWindowFlag_Background = 1 << 2,
	UIWindowFlag_Border = 1 << 3,
	UIWindowFlag_ClipContents = 1 << 4,
};

struct UIWindow
{
	u32 id;
	i32 index;
	char caption[64];
	float2 pos;
	float2 size;
	float2 containerSize;
	float2 contentSize; // Only known after filling container
	float contentOffset;
	float contentOffsetBeforeScrolling;
	bool visible;
	bool dragging;
	bool resizing;
	bool scrolling;
	int2 sizeBeforeResize;
	bool disableWidgets;
	bool clippedContents;
	u32 layer;

	UISection sections[16];
	u32 sectionCount;
};

enum UIWidgetFlags
{
	UIWidgetFlag_None = 0,
	UIWidgetFlag_Outline = (1<<0),
};

struct UIWidget
{
	float2 pos;
	float2 size;
};

enum UILayout
{
	UiLayoutVertical,
	UiLayoutHorizontal,
	UiLayoutItemBrowser,
};

struct UILayoutGroup
{
	UILayout layout;
	float2 pos;
	float2 size;
};

struct UIVertexRange
{
	u32 index;
	u32 count;
};

struct UISortKey
{
	u16 layer;
	u16 order;
};

enum UIDrawListFlags
{
	UIDrawListFlags_None = 0 << 0,
	UIDrawListFlags_Topmost = 1 << 0,
};

struct UIDrawList
{
	rect scissorRect;
	UIVertexRange vertexRanges[32];
	u32 vertexRangeCount;
	ImageH imageHandle;
	UISortKey sortKey;
};

struct UICombo
{
	u32 id;
//	const char *text;
//	const char **items;
//	u32 itemCount;
//	u32 selectedIndex;
//	bool selectedIndexChanged;
	float2 pos;
	float2 size;
};

struct UIIcon
{
	ImagePixels image;

	// Known after packed into the atlas
	int2 pos;
	float2 uv;
	float2 uvSize;
};

struct UI
{
	u32 frameIndex;

	UIVertex *frontendVertices;

	BufferH vertexBuffer[MAX_FRAMES_IN_FLIGHT];
	UIVertex *backendVertices[MAX_FRAMES_IN_FLIGHT];
	UIVertex *vertexPtr;
	u32 vertexCount;
	u32 vertexCountLimit;

	UIDrawList drawLists[32];
	u32 drawListCount;

	u32 drawListStack[32];
	u32 drawListStackSize;

	ImageH fontAtlasH;
	float2 fontAtlasSize;
	float fontScale, fontAscent, fontDescent, fontLineGap;

	float2 whitePixelUv;

	stbtt_packedchar charData[255];

	float4 colors[16];
	u32 colorCount;

	float2 cursorStack[16];
	u32 cursorStackSize;

	UIInput input;
	uint2 viewportSize;

	UIWindow windows[16];
	u32 windowCount;

	u32 windowStack[16];
	u32 windowStackSize;

	UIWindow *activeWindow;
	UIWindow *hoveredWindow;

	UIWidget widgetStack[16];
	u32 widgetStackSize;

	u32 activeWidgetId;

	UILayoutGroup layoutGroups[16];
	u32 layoutGroupCount;

	UICombo comboBox;

	bool avoidWindowInteraction;
	bool wantsInput;

	UIIcon *icons;
	u32 iconCount;

	char *tempString;

	bool menuBarBegan;
	UIWindow *activeMenu;
	UIVertex *activeMenuVertexPtr;
};

static float2 dX(float2 v)
{
	const float2 res = { v.x, 0 };
	return res;
}

static float2 dY(float2 v)
{
	const float2 res = { 0, v.y };
	return res;
}

bool UI_IsMouseClick(const UI &ui)
{
	const bool click = ui.input.mouse.buttons[MOUSE_BUTTON_LEFT] == BUTTON_STATE_PRESS;
	return click;
}

bool UI_IsMouseIdle(const UI &ui)
{
	const bool idle = ui.input.mouse.buttons[MOUSE_BUTTON_LEFT] == BUTTON_STATE_IDLE;
	return idle;
}

int2 UI_MouseScroll(const UI &ui)
{
	const int2 scroll = { .x = ui.input.mouse.wx, .y = ui.input.mouse.wy };
	return scroll;
}

int2 UI_MousePos(const UI &ui)
{
	const int2 pos = { .x = ui.input.mouse.x, .y = ui.input.mouse.y };
	return pos;
}

int2 UI_LastMouseClickPos(const UI &ui)
{
	const int2 pos = { .x = ui.input.lastMouseClickPos.x, .y = ui.input.lastMouseClickPos.y };
	return pos;
}

float2 UI_GetCursorPos(const UI &ui)
{
	ASSERT(ui.cursorStackSize > 0);
	const float2 cursorPos = ui.cursorStack[ui.cursorStackSize-1];
	return cursorPos;
}

void UI_PushCursorPos(UI &ui, float2 pos)
{
	ASSERT(ui.cursorStackSize < ARRAY_COUNT(ui.cursorStack));
	ui.cursorStack[ui.cursorStackSize++] = pos;
}

void UI_PopCursorPos(UI &ui)
{
	ASSERT(ui.cursorStackSize > 0);
	ui.cursorStackSize--;
}

void UI_SetCursorPos(UI &ui, float2 pos)
{
	ASSERT(ui.cursorStackSize > 0);
	ui.cursorStack[ui.cursorStackSize-1] = pos;
}

void UI_SetCursorPosX(UI &ui, f32 x)
{
	ASSERT(ui.cursorStackSize > 0);
	ui.cursorStack[ui.cursorStackSize-1].x = x;
}

void UI_MoveCursorDown(UI &ui, float amount)
{
	float2 cursorPos = UI_GetCursorPos(ui);
	cursorPos.y += amount;
	UI_SetCursorPos(ui, cursorPos);
}

void UI_MoveCursorRight(UI &ui, float amount)
{
	float2 cursorPos = UI_GetCursorPos(ui);
	cursorPos.x += amount;
	UI_SetCursorPos(ui, cursorPos);
}

UILayout UI_GetLayout(const UI &ui)
{
	ASSERT(ui.layoutGroupCount > 0);
	const UILayout layout = ui.layoutGroups[ui.layoutGroupCount - 1].layout;
	return layout;
}

UILayoutGroup &UI_GetLayoutGroup(UI &ui)
{
	ASSERT(ui.layoutGroupCount > 0);
	UILayoutGroup &group = ui.layoutGroups[ui.layoutGroupCount - 1];
	return group;
}

const UIDrawList &UI_GetDrawList(const UI &ui);

void UI_CursorAdvance(UI &ui, float2 prevWidgetSize, float spacing = UiSpacing)
{
	const UILayout layout = UI_GetLayout(ui);
	const UILayoutGroup group = UI_GetLayoutGroup(ui);

	if ( layout == UiLayoutHorizontal )
	{
		UI_MoveCursorRight(ui, prevWidgetSize.x + spacing);
	}
	else if ( layout == UiLayoutVertical )
	{
		UI_MoveCursorDown(ui, prevWidgetSize.y + spacing);
	}
	else if ( layout == UiLayoutItemBrowser )
	{
		const UIDrawList &drawlist = UI_GetDrawList(ui);
		const f32 prevWidgetX = UI_GetCursorPos(ui).x;
		const f32 nextWidgetEnd = prevWidgetX + prevWidgetSize.x * 2.0f;
		const f32 contentEnd = drawlist.scissorRect.pos.x + drawlist.scissorRect.size.x;
		const bool spaceAtRight = nextWidgetEnd < contentEnd;

		if ( spaceAtRight )
		{
			UI_MoveCursorRight(ui, prevWidgetSize.x + spacing);
		}
		else
		{
			UI_SetCursorPosX(ui, group.pos.x);
			UI_MoveCursorDown(ui, prevWidgetSize.y + spacing);
		}
	}
}

void UI_BeginLayout(UI &ui, UILayout layout)
{
	ASSERT(ui.layoutGroupCount < ARRAY_COUNT(ui.layoutGroups));
	UILayoutGroup &group = ui.layoutGroups[ui.layoutGroupCount++];
	group.layout = layout;
	group.pos = UI_GetCursorPos(ui);
	group.size = {};
}

void UI_EndLayout(UI &ui)
{
	ASSERT(ui.layoutGroupCount > 0);

	ui.layoutGroupCount--;

	if ( ui.layoutGroupCount )
	{
		const UILayoutGroup &endedGroup = ui.layoutGroups[ui.layoutGroupCount];
		UI_SetCursorPos(ui, endedGroup.pos);
		UI_CursorAdvance(ui, endedGroup.size);

		// Grow the current layout group
		UILayoutGroup &group = UI_GetLayoutGroup(ui);
		const float2 maxLayoutPoint = group.pos + group.size;
		const float2 maxPrevLayoutPoint = endedGroup.pos + endedGroup.size;
		if (maxPrevLayoutPoint.x > maxLayoutPoint.x) {
			group.size.x = maxPrevLayoutPoint.x - group.pos.x;
		}
		if (maxPrevLayoutPoint.y > maxLayoutPoint.y) {
			group.size.y = maxPrevLayoutPoint.y - group.pos.y;
		}
	}
}

void UI_SetInputState(UI &ui, const Keyboard &keyboard, const Mouse &mouse, const Chars &chars)
{
	UIInput &input = ui.input;

	const Mouse prevMouse = input.mouse;

	input.mouse = mouse;
	input.keyboard = keyboard;
	input.chars = chars;

	// Workaround to avoid missing some dx,dy updates...
	input.mouse.dx = mouse.x - prevMouse.x;
	input.mouse.dy = mouse.y - prevMouse.y;
}

void UI_SetViewportSize(UI &ui, uint2 size)
{
	ui.viewportSize = size;
}

u32 UI_DrawListCount(const UI &ui)
{
	return ui.drawListCount;
}

const UIDrawList &UI_GetDrawListAt(const UI &ui, u32 i)
{
	ASSERT(i < ui.drawListCount);
	return ui.drawLists[i];
}

UIDrawList &UI_GetDrawList(UI &ui)
{
	ASSERT(ui.drawListCount > 0 && ui.drawListStackSize > 0);
	const u32 drawListIndex = ui.drawListStack[ui.drawListStackSize-1];
	return ui.drawLists[drawListIndex];
}

const UIDrawList &UI_GetDrawList(const UI &ui)
{
	ASSERT(ui.drawListCount > 0 && ui.drawListStackSize > 0);
	const u32 drawListIndex = ui.drawListStack[ui.drawListStackSize-1];
	return ui.drawLists[drawListIndex];
}

void UI_PushDrawList(UI &ui, rect scissorRect, ImageH imageHandle, UIDrawListFlags flags = UIDrawListFlags_None)
{
	ASSERT(ui.drawListCount < ARRAY_COUNT(ui.drawLists));

	const u32 drawListIndex = ui.drawListCount++;

	UIDrawList &drawList = ui.drawLists[drawListIndex];
	drawList.vertexRanges[0].index = ui.vertexCount;
	drawList.vertexRanges[0].count = 0;
	drawList.vertexRangeCount = 1;
	drawList.scissorRect = scissorRect;
	drawList.imageHandle = imageHandle;

	drawList.sortKey.order = ( flags & UIDrawListFlags_Topmost ) ? U32_MAX : drawListIndex;
	if (ui.drawListStackSize > 0)
	{
		const u32 parentDrawListIndex = ui.drawListStack[ui.drawListStackSize-1];
		drawList.sortKey.layer = ui.drawLists[parentDrawListIndex].sortKey.layer;
	}

	ui.drawListStack[ui.drawListStackSize++] = drawListIndex;
}

void UI_PushDrawList(UI &ui, ImageH imageHandle)
{
	const UIDrawList &currDrawList = UI_GetDrawList(ui);
	UI_PushDrawList(ui, currDrawList.scissorRect, imageHandle);
}

void UI_PopDrawList(UI &ui)
{
	ASSERT(ui.drawListStackSize > 0);

	// Remove empty ranges in the draw list before popping
	const u32 drawListIndex = ui.drawListStack[ui.drawListStackSize-1];
	UIDrawList &drawList = ui.drawLists[drawListIndex];
	ASSERT(drawList.vertexRangeCount > 0);
	UIVertexRange &range = drawList.vertexRanges[drawList.vertexRangeCount-1];
	if (range.count == 0)
	{
		drawList.vertexRangeCount--;
	}

	ui.drawListStackSize--;

	if (ui.drawListStackSize > 0)
	{
		// Setup the next range in the parent draw list
		const u32 drawListIndex = ui.drawListStack[ui.drawListStackSize-1];
		UIDrawList &drawList = ui.drawLists[drawListIndex];
		ASSERT(drawList.vertexRangeCount > 0);
		UIVertexRange &range = drawList.vertexRanges[drawList.vertexRangeCount-1];
		if (range.count == 0)
		{
			// Reuse range, it was empty
			range.index = ui.vertexCount;
		}
		else
		{
			// Previous range was not empty, create a new one
			UIVertexRange &nextRange = drawList.vertexRanges[drawList.vertexRangeCount++];
			nextRange.index = ui.vertexCount;
			nextRange.count = 0;
		}
	}

}

void UI_RaiseWindow(UI &ui, UIWindow &window)
{
	// Push down all windows in front of the one to be raised
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		if (ui.windows[i].layer < window.layer)
		{
			ui.windows[i].layer++;
		}
	}

	// Finally put this window in front
	window.layer = 0;
}

void UI_UploadVerticesToGPU(UI &ui)
{
	const UIVertex *srcVertexBase = ui.frontendVertices;
	UIVertex *dstVertexBase = ui.backendVertices[ui.frameIndex];
	u32 totalVertexCount = 0;

	// Merge vertex ranges and copy draw list vertices to GPU
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		UIDrawList &drawList = ui.drawLists[i];
		u32 drawListVertexCount = 0;

		// Copy merged vertex ranges into a contiguous chunk of GPU memory
		for (u32 j = 0; j < drawList.vertexRangeCount; ++j)
		{
			const UIVertexRange &range = drawList.vertexRanges[j];
			const UIVertex *srcVertex = srcVertexBase + range.index;
			UIVertex *dstVertex = dstVertexBase + totalVertexCount;
			MemCopy(dstVertex, srcVertex, range.count * sizeof(UIVertex));
			drawListVertexCount += range.count;
			totalVertexCount += range.count;
		}

		// Modify draw list range info (now there's only one range)
		drawList.vertexRangeCount = 1;
		drawList.vertexRanges[0].index = totalVertexCount - drawListVertexCount;
		drawList.vertexRanges[0].count = drawListVertexCount;
	}

	ASSERT(ui.vertexCount == totalVertexCount);

}

BufferH UI_GetVertexBuffer(const UI& ui)
{
	return ui.vertexBuffer[ui.frameIndex];
}

bool UI_MouseInArea(const UI &ui, float2 pos, float2 size)
{
	const bool inArea =
			ui.input.mouse.x >= pos.x &&
			ui.input.mouse.x <  pos.x + size.x &&
			ui.input.mouse.y >= pos.y &&
			ui.input.mouse.y <  pos.y + size.y;
	return inArea;
}

UIWindow &UI_GetCurrentWindow(UI &ui)
{
	ASSERT(ui.windowStackSize > 0);
	const u32 windowIndex = ui.windowStack[ui.windowStackSize-1];
	return ui.windows[windowIndex];
}

const UIWindow &UI_GetCurrentWindow(const UI &ui)
{
	ASSERT(ui.windowStackSize > 0);
	const u32 windowIndex = ui.windowStack[ui.windowStackSize-1];
	return ui.windows[windowIndex];
}

bool UI_WindowHovered(const UI &ui)
{
	const UIWindow &currentWindow = UI_GetCurrentWindow(ui);
	const bool hovered = &currentWindow == ui.hoveredWindow;
	return hovered;
}

bool UI_WidgetHovered(const UI &ui, float2 widgetPos, float2 widgetSize)
{
	bool hover = false;
	const UIWindow &currentWindow = UI_GetCurrentWindow(ui);

	if (currentWindow.disableWidgets)
	{
		return false;
	}

	if ( UI_WindowHovered(ui) )
	{
		const rect scissor = UI_GetDrawList(ui).scissorRect;
		const float2 scissorPos = { (f32)scissor.pos.x, (f32)scissor.pos.y };
		const float2 scissorSize = { (f32)scissor.size.x, (f32)scissor.size.y };

		hover = UI_MouseInArea(ui, widgetPos, widgetSize) &&
			UI_MouseInArea(ui, scissorPos, scissorSize);
	}

	return hover;
}

bool UI_WidgetHovered(const UI &ui)
{
	bool hover = false;
	if ( ui.widgetStackSize > 0 )
	{
		const UIWidget &widget = ui.widgetStack[ui.widgetStackSize-1];
		hover = UI_WidgetHovered(ui, widget.pos, widget.size);
	}
	return hover;
}

bool UI_WidgetClicked(const UI &ui, float2 widgetPos, float2 widgetSize)
{
	const bool clicked = UI_IsMouseClick(ui) && UI_WidgetHovered(ui, widgetPos, widgetSize);
	return clicked;
}

bool UI_WidgetClicked(const UI &ui)
{
	const bool clicked = UI_IsMouseClick(ui) && UI_WidgetHovered(ui);
	return clicked;
}

float4 UI_WidgetColor(const UI &ui)
{
	const float4 res = UI_WidgetHovered(ui) ? UiColorWidgetHover : UiColorWidget;
	return res;
}

float4 UI_BoxColor(const UI &ui)
{
	const float4 res = UI_WidgetHovered(ui) ? UiColorBoxHover : UiColorBox;
	return res;
}

float4 UI_ScrollbarColor(const UI &ui)
{
	const float4 res = UI_WidgetHovered(ui) ? UiColorScrollbarHover : UiColorScrollbar;
	return res;
}

void UI_BeginWidget(UI &ui, float2 pos, float2 size, bool avoidWindowInteraction = true)
{
	ASSERT(ui.widgetStackSize < ARRAY_COUNT(ui.widgetStack));
	const UIWidget widget = {
		.pos = pos,
		.size = size,
	};
	ui.widgetStack[ui.widgetStackSize++] = widget;

	// Grow the current layout group
	UILayoutGroup &group = UI_GetLayoutGroup(ui);
	const float2 maxWidgetPoint = pos + size;
	const float2 maxLayoutPoint = group.pos + group.size;
	if (maxWidgetPoint.x > maxLayoutPoint.x) {
		group.size.x = maxWidgetPoint.x - group.pos.x;
	}
	if (maxWidgetPoint.y > maxLayoutPoint.y) {
		group.size.y = maxWidgetPoint.y - group.pos.y;
	}

	// Disable window interaction in case the widget was clicked
	const UIWindow &currentWindow = UI_GetCurrentWindow(ui);
	if ( !currentWindow.disableWidgets && UI_WidgetClicked(ui) )
	{
		ui.avoidWindowInteraction = avoidWindowInteraction;
	}
}

void UI_EndWidget(UI &ui)
{
	ASSERT(ui.widgetStackSize > 0);
	ui.widgetStackSize--;
}

void UI_PushColor(UI &ui, float4 color)
{
	ASSERT(ui.colorCount < ARRAY_COUNT(ui.colors));
	ui.colors[ui.colorCount++] = color;
}

void UI_PopColor(UI &ui)
{
	ASSERT(ui.colorCount > 1); // Avoid popping the default color
	ui.colorCount--;
}

float4 UI_GetColor(const UI &ui)
{
	const float4 color = ui.colors[ui.colorCount-1];
	return color;
}

void UI_AddTriangle(UI &ui, const UIVertex &v0, const UIVertex &v1, const UIVertex v2)
{
	ASSERT(ui.vertexCount + 3 <= ui.vertexCountLimit);
	*ui.vertexPtr++ = v0;
	*ui.vertexPtr++ = v1;
	*ui.vertexPtr++ = v2;
	ui.vertexCount += 3;

	UIDrawList &drawList = UI_GetDrawList(ui);
	UIVertexRange &vertexRange = drawList.vertexRanges[drawList.vertexRangeCount-1];
	vertexRange.count += 3;
}

void UI_AddTriangle(UI &ui, float2 p0, float2 p1, float2 p2, float4 fcolor )
{
	const float2 uv = ui.whitePixelUv;
	const rgba color  = Rgba(fcolor);
	UI_AddTriangle(ui,
			UIVertex{ p0, uv, color },
			UIVertex{ p1, uv, color },
			UIVertex{ p2, uv, color });
}

void UI_AddTriangle(UI &ui, float2 p0, float2 p1, float2 p2)
{
	const float4 color = UI_GetColor(ui);
	UI_AddTriangle(ui, p0, p1, p2, color);
}

void UI_AddQuad(UI &ui, float2 pos, float2 size, float2 uv, float2 uvSize, float4 fcolor)
{
	UIDrawList &drawList = UI_GetDrawList(ui);

	const int2 containerMin = drawList.scissorRect.pos;
	const int2 containerMax = drawList.scissorRect.pos + drawList.scissorRect.size;
	const float2 min = pos;
	const float2 max = pos + size;
	const bool outside =
		min.x >= containerMax.x || min.y >= containerMax.y ||
		max.x < containerMin.x || max.y < containerMin.y;

	if ( !outside )
	{
		// pos
		const float2 v0 = { pos.x, pos.y }; // top-left
		const float2 v1 = { pos.x, pos.y + size.y }; // bottom-left
		const float2 v2 = { pos.x + size.x, pos.y + size.y }; // bottom-right
		const float2 v3 = { pos.x + size.x, pos.y }; // top-right

		// uv
		const float2 uvTL = { uv.x, uv.y };
		const float2 uvBL = { uv.x, uv.y + uvSize.y };
		const float2 uvBR = { uv.x + uvSize.x , uv.y + uvSize.y };
		const float2 uvTR = { uv.x + uvSize.x , uv.y };

		// color
		const rgba color  = Rgba(fcolor);

		UI_AddTriangle(ui,
				UIVertex{ v0, uvTL, color },
				UIVertex{ v1, uvBL, color },
				UIVertex{ v2, uvBR, color });
		UI_AddTriangle(ui,
				UIVertex{ v0, uvTL, color },
				UIVertex{ v2, uvBR, color },
				UIVertex{ v3, uvTR, color });
	}
}

void UI_AddRectangle(UI &ui, float2 pos, float2 size)
{
	const float2 uvSize = {0, 0};
	const float4 color = UI_GetColor(ui);
	UI_AddQuad(ui, pos, size, ui.whitePixelUv, uvSize, color);
}

void UI_AddCircle(UI &ui, float2 pos, float radius)
{
	const u32 numTriangles = 8;
	const float2 center = pos + float2{radius, radius};
	for (u32 i = 0; i < numTriangles; ++i)
	{
		const float angle0 = TwoPi * (float)i / numTriangles;
		const float angle1 = TwoPi * (float)(i + 1) / numTriangles;
		const float2 v0 = center + radius * float2{Sin(angle0),Cos(angle0)};
		const float2 v1 = center + radius * float2{Sin(angle1),Cos(angle1)};
		UI_AddTriangle(ui, center, v0, v1);
	}
}

void UI_AddBorder(UI &ui, float2 pos, float2 size, float borderSize)
{
	const float2 uvSize = {0, 0};
	const float4 color = UI_GetColor(ui);
	const float2 topPos = pos;
	const float2 leftPos = pos + float2{0.0, 1.0f};
	const float2 rightPos = pos + float2{size.x-1, 1.0f};
	const float2 bottomPos = pos + float2{0.0,size.y-1};
	const float2 hSize = float2{size.x, 1.0f};
	const float2 vSize = float2{1.0, size.y - 2.0f};
	UI_AddQuad(ui, topPos, hSize, ui.whitePixelUv, uvSize, color);
	UI_AddQuad(ui, leftPos, vSize, ui.whitePixelUv, uvSize, color);
	UI_AddQuad(ui, rightPos, vSize, ui.whitePixelUv, uvSize, color);
	UI_AddQuad(ui, bottomPos, hSize, ui.whitePixelUv, uvSize, color);
}

float UI_TextHeight(const UI &ui)
{
	const float textHeight = ui.fontAscent - ui.fontDescent;
	return textHeight;
}

float UI_TextWidth(const UI &ui, const char *text, u32 maxChars = U32_MAX)
{
	float textWidth = 0.0f;

	const char *ptr = text;
	while ( *ptr && maxChars-- > 0 )
	{
		const char c = *ptr;
		const stbtt_packedchar &pc = ui.charData[c];
		textWidth += pc.xadvance;
		ptr++;
	}

	return (f32)(u32)textWidth;
}

float2 UI_AdjustTextVertically(const UI &ui, float2 pos, float height)
{
	const float textHeight = UI_TextHeight(ui);
	const float ypos = Round(pos.y + height / 2.0 - textHeight / 2.0);
	const float2 res = { pos.x, ypos };
	return res;
}

float2 UI_TextSize(const UI &ui, const char *text)
{
	const float textWidth = UI_TextWidth(ui, text);
	const float textHeight = UI_TextHeight(ui);
	const float2 textSize = { textWidth, textHeight };
	return textSize;
}

void UI_AddText(UI &ui, float2 pos, const char *text)
{
	const float cursory = Round( pos.y + ui.fontAscent );
	float cursorx = pos.x;

	const char *ptr = text;
	while ( *ptr )
	{
		const char c = *ptr;
		const stbtt_packedchar &pc = ui.charData[c];

		const float charWidth = pc.x1 - pc.x0;
		const float charHeight = pc.y1 - pc.y0;
		const float2 charPos = {cursorx + pc.xoff, cursory + pc.yoff};
		const float2 charSize = {charWidth, charHeight};
		const float2 charUv = {pc.x0/ui.fontAtlasSize.x, pc.y0/ui.fontAtlasSize.y};
		const float2 charUvSize = {charWidth/ui.fontAtlasSize.x, charHeight/ui.fontAtlasSize.y};

		UI_AddQuad(ui, charPos, charSize, charUv, charUvSize, UiColorWhite);

		cursorx = Round( cursorx + pc.xadvance );
		ptr++;
	}
}

UIWindow &UI_FindWindow(UI &ui, u32 windowId)
{
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];
		if ( window.id == windowId )
		{
			return window;
		}
	}

	ASSERT(0 && "Could not find window.");
	static UIWindow window = {};
	return window;
}

UIWindow &UI_FindOrCreateWindow(UI &ui, u32 windowId, const char *caption)
{
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];
		if ( window.id == windowId )
		{
			ASSERT(window.caption != 0);
			return window;
		}
	}

	ASSERT(ui.windowCount < ARRAY_COUNT(ui.windows));
	const u32 windowIndex = ui.windowCount++;
	UIWindow &window = ui.windows[windowIndex];

	// TODO: Remove test code to set initial window position.
	static int start = 0;
	window.id = windowId;
	window.index = windowIndex;
	StrCopy(window.caption, caption);
	window.pos = {100.0f + start*20.0f, 100.0f+start*20.0f};
	window.size = {200.0f, 300.0f};
	window.layer = windowIndex;
	UI_RaiseWindow(ui, window);
	start++;

	return window;
}

//void UI_BeginPanel(UI &ui, u32 index, float2 pos, float2 size, UIDrawListFlags flags = UIDrawListFlags_None)
//{
//	UI_PushDrawList(ui, rect{(i32)pos.x, (i32)pos.y, (u32)size.x, (u32)size.y}, ui.fontAtlasH, flags);
//}
//
//void UI_EndPanel(UI &ui)
//{
//	UI_PopDrawList(ui);
//}

void UI_BeginWindow(UI &ui, u32 windowId, u32 flags)
{
	ASSERT(ui.windowStackSize < ARRAY_COUNT(ui.windowStack));

	UIWindow &window = UI_FindWindow(ui, windowId);
	ui.windowStack[ui.windowStackSize++] = window.index;
	window.visible = true;

	UI_PushCursorPos(ui, window.pos);

	UI_PushDrawList(ui, rect{0, 0, ui.viewportSize.x, ui.viewportSize.y}, ui.fontAtlasH, UIDrawListFlags_None);

	UIDrawList &drawList = UI_GetDrawList(ui);
	drawList.sortKey.layer = window.layer;
	drawList.sortKey.order = 0;

	UI_BeginLayout(ui, UiLayoutVertical);
	float2 panelPos = window.pos;
	float2 panelSize = window.size;

	if ( flags & UIWindowFlag_Border )
	{
		UI_PushColor(ui, UiColorBorder);
		UI_AddBorder(ui, window.pos, window.size, UiBorderSize.x);
		UI_PopColor(ui);

		panelPos = panelPos + UiBorderSize;
		panelSize = panelSize - 2.0f * UiBorderSize;
	}

	if (flags & UIWindowFlag_Titlebar)
	{
		const bool activeWindow = ui.activeWindow == &window;
		const float2 titlebarPos = panelPos;
		const float2 titlebarSize = float2{panelSize.x, 18.0f};

		UI_PushColor(ui, activeWindow ? UiColorCaption : UiColorCaptionInactive);
		UI_AddRectangle(ui, titlebarPos, titlebarSize);
		UI_PopColor(ui);

		const float2 captionPos = UI_AdjustTextVertically(ui, titlebarPos + float2{UiWindowPadding.x, 0.0}, titlebarSize.y);
		UI_AddText(ui, captionPos, window.caption);

		panelPos.y += titlebarSize.y;
		panelSize.y -= titlebarSize.y;
	}

	if ( flags & UIWindowFlag_Background )
	{
		UI_PushColor(ui, UiColorPanel);
		UI_AddRectangle(ui, panelPos, panelSize);
		UI_PopColor(ui);
	}

	const float cornerSize = 14;

	if ( flags & UIWindowFlag_Resizable )
	{
		const float2 cornerBR = window.pos + window.size - (flags & UIWindowFlag_Border ? UiBorderSize : float2{0, 0});
		const float2 cornerTR = cornerBR + float2{0.0, -cornerSize};
		const float2 cornerBL = cornerBR + float2{-cornerSize, 0.0};
		const float2 cornerTL = cornerBR + float2{-cornerSize, -cornerSize};
		UI_BeginWidget(ui, cornerTL, float2{cornerSize, cornerSize}, false);
		const bool cornerHovered = UI_WidgetHovered(ui);
		UI_PushColor(ui, cornerHovered ? UiColorWidgetHover : UiColorWidget);
		UI_AddTriangle(ui, cornerBL, cornerBR, cornerTR);
		UI_PopColor(ui);
		if (UI_WidgetClicked(ui))
		{
			window.sizeBeforeResize = {(i32)window.size.x, (i32)window.size.y};
			window.resizing = true;
			// Disable other widgets, resize widget has prevalence
			window.disableWidgets = true;
		}
		UI_EndWidget(ui);
	}

	float2 cursorPos = panelPos;

	if ( flags & UIWindowFlag_ClipContents )
	{
		window.clippedContents = true;

		const u32 containerHeight = (u32)(panelSize.y - 2.0f * UiWindowPadding.y);

		const bool renderScrollbar = containerHeight < window.contentSize.y;
		const i32 scrollbarWidth = renderScrollbar ? UiWindowPadding.x : 0;
		const i32 scrollbarOuterWidth = renderScrollbar ? 3.0f * UiWindowPadding.x : 0;

		const u32 containerWidth = renderScrollbar ?
			(u32)(panelSize.x - UiWindowPadding.x - scrollbarOuterWidth) :
			(u32)(panelSize.x - 2.0f * UiWindowPadding.x - scrollbarWidth);

		const int2 containerPos = {
			.x = (i32)(panelPos.x + UiWindowPadding.x),
			.y = (i32)(panelPos.y + UiWindowPadding.y),
		};
		const uint2 containerSize = {
			.x = containerWidth,
			.y = containerHeight,
		};

		if (renderScrollbar)
		{
			if (UI_IsMouseIdle(ui)) {
				window.scrolling = false;
			}

			if (window.scrolling) { // Scroll by widget
				const f32 mouseDelta = UI_LastMouseClickPos(ui).y - UI_MousePos(ui).y;
				const f32 scrollDelta = window.contentSize.y * mouseDelta / containerHeight;
				window.contentOffset = window.contentOffsetBeforeScrolling + scrollDelta;
			} else if (UI_WindowHovered(ui)) {
				const f32 scrollDelta = UI_MouseScroll(ui).y * 16; // Scroll by mouse
				window.contentOffset -= scrollDelta;
			}

			const f32 minTopPosition = Min(window.containerSize.y - window.contentSize.y, 0.0f);
			const f32 maxTopPosition = 0.0f;
			window.contentOffset = Min(window.contentOffset, maxTopPosition);
			window.contentOffset = Max(window.contentOffset, minTopPosition);

			const f32 scrollbarPortion = Min(containerHeight / window.contentSize.y , 1.0f);
			const f32 scrollbarHeight = containerHeight * scrollbarPortion;
			const float2 scrollbarSize = { scrollbarWidth * 2.0f, scrollbarHeight };
			const f32 scrollbarMinY = containerPos.y;
			const f32 scrollbarMaxY = scrollbarMinY + containerHeight - scrollbarHeight;
			const f32 scrollbarPosYNorm = 1.0f - (window.contentOffset - minTopPosition)/(maxTopPosition - minTopPosition);
			const f32 scrollbarPosY = scrollbarMinY + (scrollbarMaxY - scrollbarMinY) * scrollbarPosYNorm;
			const float2 scrollbarPos = { (f32)containerPos.x + (f32)containerSize.x + UiWindowPadding.x * 0.5f, scrollbarPosY };
			UI_BeginWidget(ui, scrollbarPos, scrollbarSize);
			UI_PushColor(ui, UI_ScrollbarColor(ui));
			UI_AddRectangle(ui, scrollbarPos, scrollbarSize);
			UI_PopColor(ui);
			if (UI_WidgetClicked(ui)) {
				window.scrolling = true;
				window.contentOffsetBeforeScrolling = window.contentOffset;
			}
			UI_EndWidget(ui);
		}
		else
		{
			window.contentOffset = 0.0f;
		}

		const rect containerRect = { .pos = containerPos, .size = containerSize };
		UI_PushDrawList(ui, containerRect, ui.fontAtlasH);

		cursorPos = panelPos + UiWindowPadding;
		panelSize = {(f32)containerSize.x, (f32)containerSize.y};

		cursorPos.y += window.contentOffset;
	}

	window.containerSize = panelSize;

	UI_SetCursorPos(ui, cursorPos);
	UI_BeginLayout(ui, UiLayoutVertical);
}

i32 UI_MakeID(const UI &ui, const char *text)
{
	u32 parentId = 0;
	if ( ui.windowStackSize > 0 )
	{
		const UIWindow &window = UI_GetCurrentWindow(ui);
		parentId = window.id;
	}

	const u32 id = HashStringFNV(text, parentId);
	ASSERT(id != 0);
	return id;
}

void UI_BeginWindow(UI &ui, const char *caption, u32 flags = UIWindowFlag_Resizable | UIWindowFlag_Titlebar | UIWindowFlag_Border | UIWindowFlag_Background | UIWindowFlag_ClipContents )
{
	const u32 windowId = UI_MakeID(ui, caption);
	UIWindow &window = UI_FindOrCreateWindow(ui, windowId, caption);

	UI_BeginWindow(ui, windowId, flags);
}

void UI_EndWindow(UI &ui)
{
	UIWindow &window = UI_GetCurrentWindow(ui);

	window.contentSize = UI_GetLayoutGroup(ui).size;
	UI_EndLayout(ui); // Contents layout

	if (window.clippedContents)
	{
		UI_PopDrawList(ui);
	}
	UI_EndLayout(ui); // Panel layout
	UI_PopDrawList(ui);
	UI_PopCursorPos(ui);

	ASSERT(ui.windowStackSize > 0);
	ui.windowStackSize--;

}

UISection &UI_GetSection(UIWindow &window, const char *caption)
{
	const u32 sectionHash = HashStringFNV(caption);
	for (u32 i = 0; i < window.sectionCount; ++i)
	{
		if ( window.sections[i].hash == sectionHash )
		{
			return window.sections[i];
		}
	}

	ASSERT(window.sectionCount < ARRAY_COUNT(window.sections));
	UISection &section = window.sections[window.sectionCount++];
	section.hash = sectionHash;
	section.open = false;
	return section;
}

float2 UI_GetContainerSize(const UIWindow &window)
{
	return window.containerSize;
}

bool UI_Section(UI &ui, const char *caption)
{
	UIWindow &window = UI_GetCurrentWindow(ui);
	UISection &section = UI_GetSection(window, caption);

	const float containerWidth = UI_GetContainerSize(window).x;
	const float textHeight = UI_TextHeight(ui);
	constexpr f32 vpadding = 3.0f;
	const float2 pos = UI_GetCursorPos(ui);
	const float2 size = { containerWidth, textHeight + 2.0f * vpadding};

	UI_BeginWidget(ui, pos, size);

	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);

	constexpr f32 triangleHeight = 10.0f;
	constexpr float2 triangleOffset = {4.0f, 4.0f};
	const float2 p0 = pos + triangleOffset;
	const float2 p1 = p0 + ( section.open ?
		float2{triangleHeight*0.5f, triangleHeight} :
		float2{0.0f, triangleHeight} );
	const float2 p2 = p0 + ( section.open ?
		float2{triangleHeight, 0.0f} :
		float2{triangleHeight, triangleHeight * 0.5f} );
	UI_AddTriangle(ui, p0, p1, p2, UiColorWhite);

	constexpr float2 textOffset = {20.0f, 3.0f};
	const float2 textPos = pos + textOffset;
	UI_AddText(ui, textPos, caption);

	const bool clicked = UI_WidgetClicked(ui);
	if (clicked) {
		section.open = !section.open;
	}
	UI_EndWidget(ui);
	UI_CursorAdvance(ui, size);

	return section.open;
}

void UI_Label(UI &ui, const char *format, ...)
{
	UI_VSPRINTF(format, text);

	UI_AddText(ui, UI_GetCursorPos(ui), text);

	const float textHeight = UI_TextHeight(ui);
	UI_MoveCursorDown(ui, textHeight + UiSpacing);
}

bool UI_Button(UI &ui, const char *text)
{
	constexpr float2 padding = {4.0f, 3.0f};
	const float2 textSize = UI_TextSize(ui, text);
	const float2 size = textSize + 2.0f * padding;

	const float2 pos = UI_GetCursorPos(ui);

	UI_BeginWidget(ui, pos, size);

	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);

	const float2 textPos = pos + padding;
	UI_AddText(ui, textPos, text);

	const bool clicked = UI_WidgetClicked(ui);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, size);

	return clicked;
}

bool UI_ButtonIcon(UI &ui, u32 iconIndex)
{
	ASSERT(iconIndex < ui.iconCount);
	const UIIcon &icon = ui.icons[iconIndex];

	constexpr float2 padding = {2.0f, 2.0f};
	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 iconPos = widgetPos + padding;
	const float2 iconSize = { (f32)icon.image.width, (f32)icon.image.height };
	const float2 widgetSize = iconSize + 2.0f * padding;

	UI_BeginWidget(ui, widgetPos, widgetSize);

	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, widgetPos, widgetSize);
	UI_PopColor(ui);

	UI_AddQuad(ui, iconPos, iconSize, icon.uv, icon.uvSize, UiColorWhite);

	const bool clicked = UI_WidgetClicked(ui);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, widgetSize);

	return clicked;
}

bool UI_Radio(UI &ui, const char *text, bool active)
{
	const float2 ballPos = UI_GetCursorPos(ui);
	const float2 ballSize = {15.0, 15.0};
	const float2 textPos = ballPos + float2{ballSize.x + UiSpacing * 0.5f, 0.0};
	const float2 adjustedPos = UI_AdjustTextVertically(ui, textPos, ballSize.y);
	const float  textWidth = UI_TextWidth(ui, text);

	const float2 widgetPos = ballPos;
	const float2 widgetSize = ballSize + float2{UiSpacing, 0.0f} + float2{textWidth, 0.0f};
	UI_BeginWidget(ui, widgetPos, widgetSize);

	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddCircle(ui, ballPos, ballSize.y/2.0);
	UI_PopColor(ui);
	if (active)
	{
		const float2 margin = {3.0, 3.0};
		const float2 innerPos = ballPos + margin;
		const float2 innerSize = ballSize - 2.0 * margin;
		UI_PushColor(ui, UiColorWhite);
		UI_AddCircle(ui, innerPos, innerSize.y/2.0);
		UI_PopColor(ui);
	}
	const bool clicked = UI_WidgetClicked(ui);

	UI_AddText(ui, adjustedPos, text);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, widgetSize);

	return clicked;
}

bool UI_Checkbox(UI &ui, const char *text, bool *checked)
{
	ASSERT(checked != nullptr);

	const float2 boxPos = UI_GetCursorPos(ui);
	const float2 boxSize = {15.0, 15.0};
	const float2 textPos = boxPos + float2{boxSize.x + UiSpacing * 0.5f, 0.0};
	const float2 adjustedPos = UI_AdjustTextVertically(ui, textPos, boxSize.y);
	const float  textWidth = UI_TextWidth(ui, text);

	const float2 widgetPos = boxPos;
	const float2 widgetSize = boxSize + float2{UiSpacing, 0.0f} + float2{textWidth, 0.0f};
	UI_BeginWidget(ui, widgetPos, widgetSize);

	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);
	if (*checked)
	{
		const float2 margin = {3.0, 3.0};
		const float2 innerPos = boxPos + margin;
		const float2 innerSize = boxSize - 2.0 * margin;
		UI_PushColor(ui, UiColorWhite);
		UI_AddRectangle(ui, innerPos, innerSize);
		UI_PopColor(ui);
	}
	const bool clicked = UI_WidgetClicked(ui);
	if (clicked)
	{
		*checked = !*checked;
	}

	UI_AddText(ui, adjustedPos, text);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, widgetSize);

	return clicked;
}

void UI_Combo(UI &ui, const char *text, const char **items, u32 itemCount, u32 *selectedIndex)
{
	ASSERT(selectedIndex != nullptr);
	const u32 index = *selectedIndex;

	const UIWindow &window = UI_GetCurrentWindow(ui);
	const f32 containerWidth = UI_GetContainerSize(window).x;

	constexpr float2 padding = {4.0f, 3.0f};
	const f32 textHeight = UI_TextHeight(ui);

	const f32 side = textHeight + 2.0f * padding.y;

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{Round(containerWidth*0.6f), side};

	UI_BeginWidget(ui, widgetPos, widgetSize);

	const float2 boxPos = widgetPos;
	const float2 boxSize = float2{widgetSize.x - side, side};

	UI_PushColor(ui, UI_BoxColor(ui));
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);

	const float2 textPos = widgetPos + padding;
	UI_AddText(ui, textPos, items[index]);

	const float2 butPos = widgetPos + float2{boxSize.x, 0.0f};
	const float2 butSize = {side, side};

	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, butPos, butSize);
	UI_PopColor(ui);

	const f32 triangleSide = side * 0.6f;
	const float2 triangleOffset = 0.2f * float2{side, side};
	const float2 p0 = butPos + triangleOffset;
	const float2 p1 = p0 + float2{triangleSide*0.5f, triangleSide};
	const float2 p2 = p0 + float2{triangleSide, 0.0f};
	UI_AddTriangle(ui, p0, p1, p2, UiColorWhite);

	const bool mouseClick = UI_IsMouseClick(ui);
	const bool clickedInside = UI_WidgetClicked(ui);

	UI_EndWidget(ui);
	UI_CursorAdvance(ui, widgetSize);

	const float2 text2Pos = butPos + float2{butSize.x + UiSpacing, padding.y};
	UI_AddText(ui, text2Pos, text);

	const u32 comboId = UI_MakeID(ui, text);
	if ( clickedInside )
	{
		ui.comboBox.id = ui.comboBox.id == comboId ? 0 : comboId;
	}

	if (ui.comboBox.id == comboId)
	{
		const f32 textHeight = UI_TextHeight(ui);
		const f32 itemHeight = textHeight + 2.0f*padding.y;
		const float2 panelPos = boxPos + float2{0.0f, boxSize.y};
		float2 panelSize = 2.0f * UiBorderSize;
		for (u32 i = 0; i < itemCount; ++i)
		{
			const f32 itemWidth = Max( widgetSize.x, UI_TextWidth(ui, items[i]) + 2.0*padding.x );
			panelSize.x = Max(panelSize.x, itemWidth);
			panelSize.y += itemHeight;
		}

		UIWindow &comboPanel = UI_FindOrCreateWindow(ui, comboId, "$combo");
		UI_RaiseWindow(ui, comboPanel);
		comboPanel.pos = panelPos;
		comboPanel.size = panelSize;

		UI_BeginWindow(ui, comboId, UIWindowFlag_Border);

		float2 itemPos = panelPos + UiBorderSize;
		const f32 itemWidth = panelSize.x - 2.0f*UiBorderSize.x;
		const float2 itemSize = float2{itemWidth, itemHeight};
		for (u32 i = 0; i < itemCount; ++i)
		{
			const f32 textWidth = Max( widgetSize.x, UI_TextWidth(ui, items[i]) );
			UI_BeginWidget(ui, itemPos, itemSize);
			const bool itemHovered = UI_WidgetHovered(ui);
			UI_PushColor(ui, itemHovered ? UiColorBoxHover : UiColorPanel);
			UI_AddRectangle(ui, itemPos, itemSize);
			UI_PopColor(ui);
			const float2 textPos = itemPos + padding;
			UI_AddText(ui, textPos, items[i]);
			if (itemHovered && mouseClick)
			{
				*selectedIndex = i;
			}
			UI_EndWidget(ui);
			itemPos.y += itemHeight;
		}

		UI_EndWindow(ui);

		if ( mouseClick && !clickedInside ) {
			ui.comboBox.id = 0;
		}
	}

	UI_SetCursorPos(ui, widgetPos);
	UI_CursorAdvance(ui, widgetSize);
}

void UI_Separator(UI &ui)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const float containerWidth = UI_GetContainerSize(window).x;

	const float2 pos = UI_GetCursorPos(ui);
	const float2 size = { containerWidth, 1.0 };

	UI_PushColor(ui, UiColorBorder);
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);
	UI_CursorAdvance(ui, size);
}

void UI_SeparatorLabel(UI &ui, const char *format, ...)
{
	UI_VSPRINTF(format, text);

	const UIWindow &window = UI_GetCurrentWindow(ui);
	const float containerWidth = UI_GetContainerSize(window).x;

	const float2 cornerPos = UI_GetCursorPos(ui);
	const float2 size = { containerWidth, 1.0 };

	UI_BeginLayout(ui, UiLayoutHorizontal);

	const float middle = UI_TextHeight(ui) * 0.5;
	const float padding = 4.0f;
	const float2 line1Pos = cornerPos + float2{0.0f, middle};
	const float2 line1Size = { 8.0f, 1.0 };
	const float2 textPos = cornerPos + float2{line1Size.x + padding, 0.0};
	const float2 textSize = UI_TextSize(ui, text);
	const float2 line2Pos = cornerPos + float2{line1Size.x + textSize.x + 2.0f*padding, middle};
	const float2 line2Size = float2{Max(0.0f, size.x - line1Size.x - textSize.x), line1Size.y};

	UI_PushColor(ui, UiColorBorder);
	UI_AddRectangle(ui, line1Pos, line1Size);
	UI_PopColor(ui);

	UI_AddText(ui, textPos, text);

	UI_PushColor(ui, UiColorBorder);
	UI_AddRectangle(ui, line2Pos, line2Size);
	UI_PopColor(ui);

	UI_EndLayout(ui);

	UI_CursorAdvance(ui, size);
}

bool UI_Image(UI &ui, ImageH image, float2 imageSize = float2{32, 32}, UIWidgetFlags flags = UIWidgetFlag_None)
{
	const float2 borderSize = { 1, 1 };

	const float2 framePos = UI_GetCursorPos(ui);
	const float2 frameSize = imageSize + 2.0f * borderSize;

	const float2 imagePos = framePos + borderSize;

	const float2 uvPos = {0.0f, 0.0f};
	const float2 uvSize = {1.0f, 1.0f};
	UI_PushDrawList(ui, image);
	UI_BeginWidget(ui, imagePos, imageSize);
	UI_AddQuad(ui, imagePos, imageSize, uvPos, uvSize, UiColorWhite);
	const bool clicked = UI_WidgetClicked(ui);
	UI_EndWidget(ui);
	UI_CursorAdvance(ui, imageSize);
	UI_PopDrawList(ui);

	if (flags & UIWidgetFlag_Outline )
	{
		UI_PushColor(ui, UiColorOrange);
		UI_AddBorder(ui, framePos, frameSize, 1);
		UI_PopColor(ui);
	}

	return clicked;
}

void UI_SetActiveWidget(UI &ui, u32 widgetId)
{
	ui.activeWidgetId = widgetId;
}

u32 UI_IsActiveWidget(UI &ui, u32 widgetId)
{
	const bool isActive = widgetId == ui.activeWidgetId;
	return isActive;
}

enum UpdateTextAction
{
	UpdateTextNone,
	UpdateTextUpdating,
	UpdateTextDone,
	UpdateTextCancel,
};
#define TEXT_BOX_BUFER_LEN 128
UpdateTextAction UpdateText(UI &ui, char activeBuffer[TEXT_BOX_BUFER_LEN], i32 &cursorIndex)
{
	UpdateTextAction action = UpdateTextUpdating;

	const int len = StrLen(activeBuffer);
	char tmp[TEXT_BOX_BUFER_LEN];

	if ( ui.input.chars.charCount > 0 )
	{
		StrCopy(tmp, activeBuffer + cursorIndex); // Save characters after the cursor
		for (u32 i = 0; i < ui.input.chars.charCount; ++i)
		{
			activeBuffer[cursorIndex+i] = ui.input.chars.chars[i];
			cursorIndex++;
		}
		StrCopy(activeBuffer + cursorIndex, tmp);
	}
	else if ( ui.input.keyboard.keys[K_BACKSPACE] == KEY_STATE_PRESS )
	{
		if (cursorIndex > 0) {
			StrCopy(tmp, activeBuffer + cursorIndex);
			StrCopy(activeBuffer + cursorIndex - 1, tmp);
			cursorIndex--;
		}
	}
	else if ( ui.input.keyboard.keys[K_DELETE] == KEY_STATE_PRESS )
	{
		if (cursorIndex < len) {
			StrCopy(tmp, activeBuffer + cursorIndex + 1);
			StrCopy(activeBuffer + cursorIndex, tmp);
		}
	}
	else if ( KeyPress(ui.input.keyboard, K_LEFT) )
	{
		cursorIndex = Max(cursorIndex - 1, 0);
	}
	else if ( KeyPress(ui.input.keyboard, K_RIGHT) )
	{
		cursorIndex = Min(cursorIndex + 1, len);
	}
	else if ( ui.input.keyboard.keys[K_RETURN] == KEY_STATE_PRESS )
	{
		action = UpdateTextDone;
	}
	else if ( ui.input.keyboard.keys[K_ESCAPE] == KEY_STATE_PRESS )
	{
		action = UpdateTextCancel;
	}
	else
	{
		action = UpdateTextNone;
	}

	return action;
}

void UI_InputText(UI &ui, const char *label, char *buffer, u32 bufferSize)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const f32 containerWidth = UI_GetContainerSize(window).x;

	constexpr float2 padding = {4.0f, 3.0f};
	const f32 textHeight = UI_TextHeight(ui);

	const f32 side = textHeight + 2.0f * padding.y;

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{Round(containerWidth*0.6f), side};

	UI_BeginWidget(ui, widgetPos, widgetSize);

	const float2 boxPos = widgetPos;
	const float2 boxSize = widgetSize;

	UI_PushColor(ui, UI_BoxColor(ui));
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);

	const float2 textPos = boxPos + padding;

	const u32 id = UI_MakeID(ui, label);

	static char activeBuffer[128] = {};
	static Clock activeBeginClock;
	static i32 cursorIndex;

	const bool clicked = UI_WidgetClicked(ui);
	const bool active = UI_IsActiveWidget(ui, id);
	if (clicked && !active)
	{
		UI_SetActiveWidget(ui, id);
		StrCopy(activeBuffer, buffer);
		activeBeginClock = GetClock();
		cursorIndex = StrLen(activeBuffer);
	}

	if (active)
	{
		const UpdateTextAction action = UpdateText(ui, activeBuffer, cursorIndex);

		if ( action == UpdateTextDone )
		{
			StrCopy(buffer, activeBuffer);
			UI_SetActiveWidget(ui, 0);
		}
		else if ( action == UpdateTextCancel )
		{
			UI_SetActiveWidget(ui, 0);
		}
		else if ( action == UpdateTextUpdating )
		{
			activeBeginClock = GetClock();
		}

		const Clock currentClock = GetClock();
		const float secondsActive = GetSecondsElapsed(activeBeginClock, currentClock);

		const bool printCursor = (int)(secondsActive * 3) % 3 != 2; // Show 2/3 second, hide 1/3 second
		if ( printCursor )
		{
			const f32 textWidth = UI_TextWidth(ui, activeBuffer, cursorIndex);
			const float2 cursorPos = textPos + float2{textWidth + 1.0f, 0.0f};
			const float2 cursorSize = {1.0f, textHeight};
			UI_PushColor(ui, UiColorWhite);
			UI_AddRectangle(ui, cursorPos, cursorSize);
			UI_PopColor(ui);
		}
	}

	const char *text = active ? activeBuffer : buffer;
	UI_AddText(ui, textPos, text);

	const bool mouseClick = UI_IsMouseClick(ui);
	const bool clickedInside = UI_WidgetClicked(ui);

	UI_EndWidget(ui);

	const float2 text2Pos = widgetPos + float2{widgetSize.x + UiSpacing, padding.y};
	UI_AddText(ui, text2Pos, label);

	UI_CursorAdvance(ui, widgetSize);
}

void UI_InputInt(UI &ui, const char *label, i32 *number)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const f32 containerWidth = UI_GetContainerSize(window).x;

	constexpr float2 padding = {4.0f, 3.0f};
	const f32 textHeight = UI_TextHeight(ui);

	const f32 side = textHeight + 2.0f * padding.y;

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{Round(containerWidth*0.6f), side};

	const float2 boxPos = widgetPos;
	const float2 boxSize = float2{widgetSize.x - 2.0f*side - padding.x*2.0f, side};
	UI_BeginWidget(ui, boxPos, boxSize);
	UI_PushColor(ui, UI_BoxColor(ui));
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);
	UI_EndWidget(ui);

	const float2 minusPos = boxPos + float2{boxSize.x + padding.x, 0.0f};
	const float2 minusSize = float2{side, side};
	UI_BeginWidget(ui, minusPos, minusSize);
	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, minusPos, minusSize);
	UI_PopColor(ui);
	UI_PushColor(ui, UiColorWhite);
	UI_AddRectangle(ui, float2{minusPos.x + minusSize.x*0.15f,minusPos.y + minusSize.y*0.4f}, float2{minusSize.x*0.7f,minusSize.y*0.2f});
	UI_PopColor(ui);
	if ( UI_WidgetClicked(ui) ) {
		(*number)--;
	}
	UI_EndWidget(ui);

	const float2 plusPos = minusPos + float2{minusSize.x + padding.x, 0.0f};
	const float2 plusSize = float2{side, side};
	UI_BeginWidget(ui, plusPos, plusSize);
	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, plusPos, plusSize);
	UI_PopColor(ui);
	UI_PushColor(ui, UiColorWhite);
	UI_AddRectangle(ui, float2{plusPos.x + plusSize.x*0.15f,plusPos.y + plusSize.y*0.4f}, float2{plusSize.x*0.7f,plusSize.y*0.2f});
	UI_AddRectangle(ui, float2{plusPos.x + plusSize.x*0.4f,plusPos.y + plusSize.y*0.15f}, float2{plusSize.x*0.2f,plusSize.y*0.7f});
	UI_PopColor(ui);
	if ( UI_WidgetClicked(ui) ) {
		(*number)++;
	}
	UI_EndWidget(ui);

	// Number
	char numberText[TEXT_BOX_BUFER_LEN];
	SPrintf(numberText, "%d", *number);
	const float2 textPos = boxPos + padding;
	UI_AddText(ui, textPos, numberText);

	// Label
	const float2 labelPos = plusPos + float2{plusSize.x + UiSpacing, padding.y};
	UI_AddText(ui, labelPos, label);

	UI_CursorAdvance(ui, widgetSize);
}

void UI_InputFloat(UI &ui, const char *label, f32 *number)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const f32 containerWidth = UI_GetContainerSize(window).x;

	constexpr float2 padding = {4.0f, 3.0f};
	const f32 textHeight = UI_TextHeight(ui);

	const f32 side = textHeight + 2.0f * padding.y;

	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{Round(containerWidth*0.6f), side};

	const float2 boxPos = widgetPos;
	const float2 boxSize = float2{widgetSize.x - 2.0f*side - padding.x*2.0f, side};
	UI_BeginWidget(ui, boxPos, boxSize);
	UI_PushColor(ui, UI_BoxColor(ui));
	UI_AddRectangle(ui, boxPos, boxSize);
	UI_PopColor(ui);
	UI_EndWidget(ui);

	const float2 minusPos = boxPos + float2{boxSize.x + padding.x, 0.0f};
	const float2 minusSize = float2{side, side};
	UI_BeginWidget(ui, minusPos, minusSize);
	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, minusPos, minusSize);
	UI_PopColor(ui);
	UI_PushColor(ui, UiColorWhite);
	UI_AddRectangle(ui, float2{minusPos.x + minusSize.x*0.15f,minusPos.y + minusSize.y*0.4f}, float2{minusSize.x*0.7f,minusSize.y*0.2f});
	UI_PopColor(ui);
	if ( UI_WidgetClicked(ui) ) {
		(*number) -= 0.1f;
	}
	UI_EndWidget(ui);

	const float2 plusPos = minusPos + float2{minusSize.x + padding.x, 0.0f};
	const float2 plusSize = float2{side, side};
	UI_BeginWidget(ui, plusPos, plusSize);
	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, plusPos, plusSize);
	UI_PopColor(ui);
	UI_PushColor(ui, UiColorWhite);
	UI_AddRectangle(ui, float2{plusPos.x + plusSize.x*0.15f,plusPos.y + plusSize.y*0.4f}, float2{plusSize.x*0.7f,plusSize.y*0.2f});
	UI_AddRectangle(ui, float2{plusPos.x + plusSize.x*0.4f,plusPos.y + plusSize.y*0.15f}, float2{plusSize.x*0.2f,plusSize.y*0.7f});
	UI_PopColor(ui);
	if ( UI_WidgetClicked(ui) ) {
		(*number) += 0.1f;
	}
	UI_EndWidget(ui);

	// Number
	char numberText[TEXT_BOX_BUFER_LEN];
	SPrintf(numberText, "%f", *number);
	const float2 textPos = boxPos + padding;
	UI_AddText(ui, textPos, numberText);

	// Label
	const float2 labelPos = plusPos + float2{plusSize.x + UiSpacing, padding.y};
	UI_AddText(ui, labelPos, label);

	UI_CursorAdvance(ui, widgetSize);
}

void UI_Histogram(UI &ui, const float *values, u32 valueCount, f32 maxValue = 1000.0f/120.0f)
{
	UIWindow &window = UI_GetCurrentWindow(ui);
	const f32 histogramWidth = UI_GetContainerSize(window).x;
	const f32 histogramHeight = 30.0f;
	const float2 histPos = UI_GetCursorPos(ui);
	const float2 histSize = {histogramWidth, histogramHeight};

	UI_BeginWidget(ui, histPos, histSize, false);

	const f32 barWidth = histogramWidth / valueCount;
	const float2 barBase = histPos + float2{0.0f, histSize.y};

	UI_PushColor(ui, UiColorOrange);
	for (u32 i = 0; i < valueCount; ++i)
	{
		const float heightRatio = values[i] / maxValue;
		const f32 barHeight = Max(1.0f , Min(heightRatio * histogramHeight, histogramHeight));
		const float2 barPos = barBase + float2{ i * barWidth, -barHeight };
		const float2 barSize = {barWidth - 1, barHeight};
		UI_AddRectangle(ui, barPos, barSize);
	}
	UI_PopColor(ui);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, histSize);
}

bool UI_BeginMenuBar(UI &ui)
{
	ASSERT(ui.windowStackSize == 0);
	ui.menuBarBegan = true;

	const char *idString = "UI_BeginMenuBar";
	const u32 windowId = UI_MakeID(ui, idString);
	UIWindow &window = UI_FindOrCreateWindow(ui, windowId, idString);
	UI_BeginWindow(ui, windowId, UIWindowFlag_None);

	window.pos = {0, 0};
	window.size = {(f32)ui.viewportSize.x, 22.0f};

	const float2 pos = window.pos;
	const float2 size = window.size;

	UI_PushColor(ui, UiColorMenu);
	UI_AddRectangle(ui, pos, size);

	const float2 borderPos = pos + float2{0.0f, size.y};
	const float2 borderSize = float2{size.x, 1.0f};
	UI_PushColor(ui, UiColorBorder);
	UI_AddRectangle(ui, borderPos, borderSize);
	UI_PopColor(ui);

	UI_SetCursorPos( ui, pos + float2{UiSpacing, 0.0f} );
	UI_BeginLayout(ui, UiLayoutHorizontal);

	return true;
}

void UI_EndMenuBar(UI &ui)
{
	ASSERT(ui.menuBarBegan);
	ui.menuBarBegan = false;

	UI_EndLayout(ui);
	UI_PopColor(ui);
	UI_EndWindow(ui);
}

bool UI_BeginMenu(UI &ui, const char *name)
{
	constexpr float2 margin = {8.0f, 3.0f};

	const float2 textSize = UI_TextSize(ui, name);
	const float2 itemSize = textSize + 2.0f * margin;

	const float2 itemPos = UI_GetCursorPos(ui);
	const float2 textPos = itemPos + margin;

	UI_BeginWidget(ui, itemPos, itemSize);
	const bool clicked = UI_WidgetClicked(ui);
	const bool hovered = UI_WidgetHovered(ui);
	UI_PushColor(ui, hovered ? UiColorMenuHover : UiColorMenu);
	UI_AddRectangle(ui, itemPos, itemSize);
	UI_PopColor(ui);
	UI_AddText(ui, textPos, name);
	UI_EndWidget(ui);

	UI_CursorAdvance(ui, itemSize, 0.0f);

	const u32 windowId = UI_MakeID(ui, name);
	UIWindow &window = UI_FindOrCreateWindow(ui, windowId, name);

	// Show on click
	if (clicked) {
		if ( ui.activeMenu == &window ) {
			ui.activeMenu = nullptr;
		} else {
			ui.activeMenu = &window;
		}
	}

	// Show on hovering another menu
	if (hovered) {
		if (ui.activeMenu != nullptr) {
			ui.activeMenu = &window;
		}
	}

	const bool showMenu = ui.activeMenu == &window;
	if ( showMenu )
	{
		window.pos = itemPos + dY(itemSize);
		UI_BeginWindow(ui, windowId, UIWindowFlag_None);
		UI_RaiseWindow(ui, window);

		// Add the rectangle background, save its vertices to modify the size later
		ui.activeMenuVertexPtr = ui.vertexPtr;
		const float2 tempSize = {0, 0};
		UI_AddRectangle(ui, window.pos, tempSize );
	}

	return showMenu;
}

void UI_EndMenu(UI &ui)
{
	UILayoutGroup &layoutGroup = UI_GetLayoutGroup(ui);
	UIWindow &window = UI_GetCurrentWindow(ui);
	window.size = layoutGroup.size;

	// Set the size of the menu background panel
	// Tri 1
	ui.activeMenuVertexPtr[1].position.y += window.size.y;
	ui.activeMenuVertexPtr[2].position.x += window.size.x;
	ui.activeMenuVertexPtr[2].position.y += window.size.y;
	// Tri 2
	ui.activeMenuVertexPtr[4].position.x += window.size.x;
	ui.activeMenuVertexPtr[4].position.y += window.size.y;
	ui.activeMenuVertexPtr[5].position.x += window.size.x;

	UI_EndWindow(ui);
}

bool UI_MenuItem(UI &ui, const char *name, bool checked = false)
{
	constexpr float2 padding = {8.0f, 4.0f};
	const float2 textSize = UI_TextSize(ui, name);
	const float2 checkSize = float2{textSize.y, textSize.y};
	const float2 widgetPos = UI_GetCursorPos(ui);
	const float2 widgetSize = float2{ textSize.x, textSize.y } + dX(padding) + dX(checkSize) + 2.0f * padding;

	UI_BeginWidget(ui, widgetPos, widgetSize);

	const UIWindow &menu = UI_GetCurrentWindow(ui);
	const float2 extWidgetSize = Max(widgetSize, dY(widgetSize) + dX(menu.size));
	const bool hovered = UI_WidgetHovered(ui, widgetPos, extWidgetSize);
	if ( hovered )
	{
		UI_PushColor(ui, UiColorMenuHover);
		UI_AddRectangle(ui, widgetPos, extWidgetSize);
		UI_PopColor(ui);
	}

	const bool clicked = UI_WidgetClicked(ui, widgetPos, extWidgetSize);
	UI_EndWidget(ui);

	const float2 textPos = widgetPos + padding;
	UI_AddText(ui, textPos, name);

	if (checked)
	{
		UI_PushColor(ui, UiColorWhite);
		const float2 n0 = { 0.131, 1.0 - 0.592};
		const float2 n1 = { 0.0, 1.0 - 0.4};
		const float2 n2 = { 0.493, 1.0 - 0.052};
		const float2 n3 = { 1.0, 1.0 - 0.778};
		const float2 n4 = { 0.81, 1.0 - 0.991};
		const float2 n5 = { 0.4364, 1.0 - 0.3771};
		const float2 checkPos = widgetPos + padding + dX(textSize) + dX(padding);
		const float2 p0 = checkPos + checkSize * n0;
		const float2 p1 = checkPos + checkSize * n1;
		const float2 p2 = checkPos + checkSize * n2;
		const float2 p3 = checkPos + checkSize * n3;
		const float2 p4 = checkPos + checkSize * n4;
		const float2 p5 = checkPos + checkSize * n5;
		UI_AddTriangle(ui, p0, p1, p2, UiColorWhite );
		UI_AddTriangle(ui, p0, p2, p5, UiColorWhite );
		UI_AddTriangle(ui, p2, p3, p4, UiColorWhite );
		UI_AddTriangle(ui, p5, p2, p4, UiColorWhite );
		UI_PopColor(ui);
	}

	UI_CursorAdvance(ui, widgetSize, 0);

	if (clicked) {
		ui.activeMenu = nullptr;
	}

	return clicked;
}

// TODO: We should depend only on tools_gfx.h while this is a feature in engine.cpp.
struct Graphics;
ImageH CreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels);

void UI_Initialize(UI &ui, Graphics &gfx, GraphicsDevice &gfxDev, Arena &globalArena, UIIcon *icons, u32 iconCount)
{
	ui.tempString = PushArray(globalArena, char, UI_TEMP_STRING_SIZE);

	const u32 vertexBufferSize = KB(128);
	ui.vertexCountLimit = vertexBufferSize / sizeof(UIVertex);

	UIVertex *vertexData = (UIVertex*)AllocateVirtualMemory(vertexBufferSize);
	ui.frontendVertices = vertexData;

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		ui.vertexBuffer[i] = CreateBuffer(
			gfxDev,
			vertexBufferSize,
			BufferUsageVertexBuffer,
			HeapType_Dynamic);

		ui.backendVertices[i] = (UIVertex*)GetBufferPtr(gfxDev, ui.vertexBuffer[i]);
	}

	UI_PushColor(ui, UiColorWidget);

	struct SizedFont
	{
		const char *filename;
		i32 height;
	};

	const SizedFont fonts[] = {
		{ "editor/fonts/proggy/ProggyClean.ttf", 13 },
		{ "editor/fonts/refixedsys/refixedsys-mono.ttf", 15 },
	};

	const SizedFont sizedFont = fonts[1];

	Arena scratch = globalArena;
	#define globalArena invalidArena // NOTE: Cannot use globalArena after scratching it

	// Load TTF font texture
	FilePath fontPath = MakePath(ProjectDir, sizedFont.filename);
	DataChunk *chunk = PushFile( scratch, fontPath.str );
	if ( !chunk )
	{
		LOG(Error, "Could not open file %s\n", fontPath.str);
		return;
	}
	const byte *fontData = chunk->bytes;

	const u32 fontAtlasWidth = 128;
	const u32 fontAtlasHeight = 128;
	byte *fontAtlasBitmap = PushArray(scratch, byte, fontAtlasWidth * fontAtlasHeight);

	stbtt_fontinfo font;
	const int fontIndex = 0;
	const int fontOffset = stbtt_GetFontOffsetForIndex(fontData, fontIndex);
	const f32 pixelHeight = (f32)sizedFont.height;
	if ( stbtt_InitFont(&font, fontData, fontOffset) )
	{
		ui.fontScale = stbtt_ScaleForPixelHeight(&font, pixelHeight);
		int fontAscent, fontDescent, fontLineGap;
		stbtt_GetFontVMetrics(&font, &fontAscent, &fontDescent, &fontLineGap);
		ui.fontAscent = fontAscent * ui.fontScale;
		ui.fontDescent = fontDescent * ui.fontScale;
		ui.fontLineGap = fontLineGap * ui.fontScale;
	}

	// Begin packing
	stbtt_pack_context packContext;
	const int strideInBytes = fontAtlasWidth; // Could be 0 to indicate 'tightly packed'
	const int padding = 1;
	void *allocContext = nullptr;
	const int res = stbtt_PackBegin(&packContext, fontAtlasBitmap, fontAtlasWidth, fontAtlasHeight, strideInBytes, padding, allocContext);

	// Pack Latin1 font range
	const int firstChar = 32;
	const int charCount = 96;
	stbtt_pack_range packRange = {
		.font_size = pixelHeight,
		.first_unicode_codepoint_in_range = firstChar,
		.array_of_unicode_codepoints = nullptr,
		.num_chars = charCount,
		.chardata_for_range = ui.charData + firstChar,
	};
	const int res2 = stbtt_PackFontRanges(&packContext, fontData, fontIndex, &packRange, 1);

	// Pack custom white pixel
	stbrp_rect whitePixelRect = {
		.id = 0, // ignored by us
		.w = 1, .h = 1, // input
		// .x = 99, .y = 99, // output
		// .was_packed = false, // output
	};
	stbtt_PackFontRangesPackRects(&packContext, &whitePixelRect, 1);
	fontAtlasBitmap[strideInBytes * whitePixelRect.y + whitePixelRect.x] = 0xFF;
	ui.whitePixelUv = float2{ (whitePixelRect.x + 0.5f) / fontAtlasWidth, (whitePixelRect.y + 0.5f)/ fontAtlasHeight };

	// Pack RGBA icons
	for (u32 i = 0; i < iconCount; ++i)
	{
		UIIcon &icon = icons[i];

		// Pack custom white pixel
		stbrp_rect iconRect = {
			.id = 0, // ignored by us
			.w = icon.image.width, .h = icon.image.height, // input
			// .x = 99, .y = 99, // output
			// .was_packed = false, // output
		};
		stbtt_PackFontRangesPackRects(&packContext, &iconRect, 1);
		ASSERT(iconRect.was_packed);

		// Save packing info
		icon.pos = int2{ iconRect.x, iconRect.y };
		icon.uv = float2{ (f32)iconRect.x/fontAtlasWidth, (f32)iconRect.y/fontAtlasHeight };
		icon.uvSize = float2{ (f32)iconRect.w/fontAtlasWidth, (f32)iconRect.h/fontAtlasHeight };
	}

	ui.icons = icons;
	ui.iconCount = iconCount;

	// End packing
	stbtt_PackEnd(&packContext);

	// One channel to RGBA
	rgba *fontAtlasBitmapRGBA = PushArray(scratch, rgba, fontAtlasWidth * fontAtlasHeight);

	byte *srcPtr = fontAtlasBitmap;
	rgba *dstPtr = fontAtlasBitmapRGBA;
	for (u32 i = 0; i < fontAtlasWidth * fontAtlasHeight; ++i)
	{
		*dstPtr++ = rgba{255, 255, 255, *srcPtr++};
	}

	// Blit icon RGBA pixels
	const u32 pixelSize = 4;
	for (u32 i = 0; i < iconCount; ++i)
	{
		const UIIcon &icon = icons[i];
		const u32 iconStride = icon.image.width * pixelSize;

		for (u32 y = 0; y < icon.image.height; ++y)
		{
			for (u32 x = 0; x < icon.image.width; ++x)
			{
				const rgba color = {
					icon.image.pixels[y * iconStride + x * pixelSize + 0],
					icon.image.pixels[y * iconStride + x * pixelSize + 1],
					icon.image.pixels[y * iconStride + x * pixelSize + 2],
					icon.image.pixels[y * iconStride + x * pixelSize + 3],
				};
				fontAtlasBitmapRGBA[strideInBytes * (icon.pos.y + y) + icon.pos.x + x] = color;
			}
		}
	}

	// Create texture
	ui.fontAtlasH = CreateImage(gfx, "texture_font", fontAtlasWidth, fontAtlasHeight, 4, false, (byte*)fontAtlasBitmapRGBA);
	ui.fontAtlasSize = {fontAtlasWidth, fontAtlasHeight};

	#undef globalArena
}

void UI_BeginFrame(UI &ui)
{
	ui.frameIndex = ( ui.frameIndex + 1 ) % MAX_FRAMES_IN_FLIGHT;
	ui.vertexPtr = ui.frontendVertices;
	ui.vertexCount = 0;
	ui.drawListCount = 0;
	ui.drawListStackSize = 0;

	if (UI_IsMouseClick(ui))
	{
		ui.input.lastMouseClickPos = UI_MousePos(ui);
	}

	// Resize / move window interactions
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];

		if ( UI_IsMouseIdle(ui) )
		{
			window.dragging = false;
			window.resizing = false;
			window.disableWidgets = false;
		}

		// If no other widget interaction blocked the window...
		if ( !ui.avoidWindowInteraction )
		{
			if ( window.resizing )
			{
				constexpr float2 minWindowSize = { 100.0f, 100.0f };
				const int2 resizeDelta = UI_MousePos(ui) - UI_LastMouseClickPos(ui);
				const int2 newWindowSize = window.sizeBeforeResize + resizeDelta;
				window.size = Max(minWindowSize, float2{(float)newWindowSize.x, (float)newWindowSize.y});
			}
			else if ( window.dragging )
			{
				window.pos += float2{(float)ui.input.mouse.dx, (float)ui.input.mouse.dy};
			}
		}
	}

	// Update hovered window
	ui.hoveredWindow = nullptr;
	u32 hoveredWindowLayer = U32_MAX;
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];

		if ( window.visible && UI_MouseInArea(ui, window.pos, window.size) && window.layer < hoveredWindowLayer )
		{
			ui.hoveredWindow = &window;
			hoveredWindowLayer = window.layer;
		}
	}

	// Update active window
	if ( UI_IsMouseClick(ui) )
	{
		if ( ui.hoveredWindow && ui.hoveredWindow->visible )
		{
			UI_RaiseWindow(ui, *ui.hoveredWindow);
			ui.activeWindow = ui.hoveredWindow;
			ui.activeWindow->dragging = true;
			ui.wantsInput = true;
		}
		else
		{
			ui.activeWindow = nullptr;
			ui.wantsInput = false;
		}

		ui.activeWidgetId = 0;
	}

	// Deactivate menus on click outside
	if ( ui.activeMenu )
	{
		const float2 pos = ui.activeMenu->pos;
		const float2 size = ui.activeMenu->size;
		if ( UI_IsMouseClick(ui) && !UI_MouseInArea(ui, pos, size)  )
		{
			ui.activeMenu = nullptr;
		}
	}

	// Deactivate menus on move mouse away
	if ( ui.activeMenu )
	{
		const float2 margin = {50, 50};
		const float2 extendedPos = ui.activeMenu->pos - margin;
		const float2 extendedSize = ui.activeMenu->size + 2.0f * margin;
		if ( !UI_MouseInArea(ui, extendedPos, extendedSize) )
		{
			ui.activeMenu = nullptr;
		}
	}

	// Reset visibility (to be determined again during this frame)
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];
		window.visible = false;
	}
}

void UI_EndFrame(UI &ui)
{
	// Create an array of draw list indices
	u32 sortedDrawListIndices[ARRAY_COUNT(ui.drawLists)];
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		sortedDrawListIndices[i] = i;
	}

	// Sort draw lists
	u32 numSwaps = U32_MAX;
	while (numSwaps > 0)
	{
		numSwaps = 0;
		for (u32 i = 1; i < ui.drawListCount; ++i)
		{
			const u32 index0 = sortedDrawListIndices[i-1];
			const u32 index1 = sortedDrawListIndices[i];
			const u32 layer0 = ui.drawLists[index0].sortKey.layer;
			const u32 layer1 = ui.drawLists[index1].sortKey.layer;
			const u32 order0 = ui.drawLists[index0].sortKey.order;
			const u32 order1 = ui.drawLists[index1].sortKey.order;
			if ( layer0 < layer1 || ( layer0 == layer1 &&  order0 > order1 ) )
			{
				sortedDrawListIndices[i-1] = index1;
				sortedDrawListIndices[i] = index0;
				numSwaps++;
			}
		}
	}

	// Copy the sorted draw lists to tmp array and back
	UIDrawList sortedDrawLists[ARRAY_COUNT(ui.drawLists)];
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		const u32 sortedDrawListIndex = sortedDrawListIndices[i];
		sortedDrawLists[i] = ui.drawLists[sortedDrawListIndex];
	}
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		ui.drawLists[i] = sortedDrawLists[i];
	}

	if ( UI_IsMouseIdle(ui) )
	{
		ui.avoidWindowInteraction = false;
	}
}

void UI_Cleanup(const UI &ui)
{
	// Nothing to do as gfx resources are cleaned up on gfx lib shutdown
}

#endif // #ifndef TOOLS_UI_H
