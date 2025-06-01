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

constexpr float4 UiColorWhite = { 1.0, 1.0, 1.0, 1.0 };
constexpr float4 UiColorBorder = { 0.3, 0.3, 0.3, 0.9 };
constexpr float4 UiColorCaption = { 0.1, 0.2, 0.4, 0.8 };
constexpr float4 UiColorPanel = { 0.05, 0.05, 0.05, 0.9 };
constexpr float4 UiColorWidget = { 0.1, 0.2, 0.4, 0.8 };
constexpr float4 UiColorWidgetHover = { 0.2, 0.4, 0.8, 1.0 };
constexpr float2 UiBorderSize = { 1.0, 1.0 };
constexpr float2 UiWindowPadding = { 8.0f, 8.0f };
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
};

struct UIWindow
{
	char caption[64];
	float2 pos;
	float2 size;
	bool dragging;
	bool resizing;
	u32 layer;
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

struct UIDrawList
{
	urect scissorRect;
	UIVertexRange vertexRanges[32];
	u32 vertexRangeCount;
	ImageH imageHandle;
	UISortKey sortKey;
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

	float2 currentPos;

	UIInput input;
	uint2 viewportSize;

	UIWindow windows[16];
	u32 windowCount;

	UIWindow *activeWindow;
	UIWindow *hoveredWindow;
	UIWindow *currentWindow;

	UIWidget widgetStack[16];
	u32 widgetStackSize;

	UILayoutGroup layoutGroups[16];
	u32 layoutGroupCount;

	bool avoidWindowInteraction;
	bool wantsMouseInput;
};

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

void UI_SetCursorPos(UI &ui, float2 pos)
{
	ui.currentPos = pos;
}

void UI_MoveCursorDown(UI &ui, float amount)
{
	ui.currentPos.y += amount;
}

void UI_MoveCursorRight(UI &ui, float amount)
{
	ui.currentPos.x += amount;
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

void UI_CursorAdvance(UI &ui, float2 prevWidgetSize)
{
	const UILayout layout = UI_GetLayout(ui);

	if ( layout == UiLayoutHorizontal )
	{
		UI_MoveCursorRight(ui, prevWidgetSize.x + UiSpacing);
	}
	else // if ( layout == UiLayoutVertical )
	{
		UI_MoveCursorDown(ui, prevWidgetSize.y + UiSpacing);
	}
}

void UI_BeginLayout(UI &ui, UILayout layout)
{
	ASSERT(ui.layoutGroupCount < ARRAY_COUNT(ui.layoutGroups));
	UILayoutGroup &group = ui.layoutGroups[ui.layoutGroupCount++];
	group.layout = layout;
	group.pos = ui.currentPos;
	group.size = {};
}

void UI_EndLayout(UI &ui)
{
	ASSERT(ui.layoutGroupCount > 0);

	ui.layoutGroupCount--;

	if ( ui.layoutGroupCount )
	{
		const UILayoutGroup &group = ui.layoutGroups[ui.layoutGroupCount];
		UI_SetCursorPos(ui, group.pos);
		UI_CursorAdvance(ui, group.size);
	}
}

void UI_SetMouseState(UI &ui, const Mouse &mouse)
{
	UIInput &input = ui.input;
	const Mouse prevMouse = input.mouse;
	input.mouse = mouse;

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

void UI_PushDrawList(UI &ui, urect scissorRect, ImageH imageHandle)
{
	ASSERT(ui.drawListCount < ARRAY_COUNT(ui.drawLists));

	const u32 drawListIndex = ui.drawListCount++;

	UIDrawList &drawList = ui.drawLists[drawListIndex];
	drawList.vertexRanges[0].index = ui.vertexCount;
	drawList.vertexRanges[0].count = 0;
	drawList.vertexRangeCount = 1;
	drawList.scissorRect = scissorRect;
	drawList.imageHandle = imageHandle;

	drawList.sortKey.order = drawListIndex;
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

void UI_BeginWidget(UI &ui, float2 pos, float2 size)
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
}

void UI_EndWidget(UI &ui)
{
	ASSERT(ui.widgetStackSize > 0);
	ui.widgetStackSize--;
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

bool UI_WidgetHovered(const UI &ui)
{
	bool hover = false;

	if ( ui.currentWindow == ui.hoveredWindow && ui.widgetStackSize > 0 )
	{
		const UIWidget widget = ui.widgetStack[ui.widgetStackSize-1];

		hover = UI_MouseInArea(ui, widget.pos, widget.size);
	}

	return hover;
}

bool UI_WidgetClicked(const UI &ui)
{
	const bool clicked = UI_WidgetHovered(ui) && UI_IsMouseClick(ui);
	return clicked;
}

float4 UI_WidgetColor(const UI &ui)
{
	const float4 res = UI_WidgetHovered(ui) ? UiColorWidgetHover : UiColorWidget;
	return res;
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
	const float2 vSize = float2{1.0, size.y};
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

float UI_TextWidth(const UI &ui, const char *text)
{
	float textWidth = 0.0f;

	const char *ptr = text;
	while ( *ptr )
	{
		const char c = *ptr;
		const stbtt_packedchar &pc = ui.charData[c];
		textWidth += pc.xadvance;
		ptr++;
	}

	return textWidth;
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

UIWindow &UI_GetWindowAtLayer(UI &ui, u32 layer)
{
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];
		if (window.layer == layer)
		{
			return window;
		}
	}

	ASSERT(0 && "Window not found.");
	static UIWindow nullWindow = { };
	return nullWindow;
}

UIWindow &UI_GetWindow(UI &ui, const char *caption)
{
	for (u32 i = 0; i < ARRAY_COUNT(ui.windows); ++i)
	{
		UIWindow &window = ui.windows[i];
		if (*window.caption == '\0')
		{
			// TODO: Remove test code to set initial window position.
			static int start = 0;
			StrCopy(window.caption, caption);
			window.pos = {100.0f + start*100.0f, 100.0f+start*100.0f};
			window.size = {200.0f, 300.0f};
			window.layer = ui.windowCount++;
			UI_RaiseWindow(ui, window);
			start++;
			return window;
		}
		else if (StrEq(window.caption, caption))
		{
			return window;
		}
	}

	ASSERT(0 && "Window not found.");
	static UIWindow nullWindow = { };
	return nullWindow;
}

void UI_BeginWindow(UI &ui, const char *caption)
{
	ASSERT(!ui.currentWindow);
	UIWindow &window = UI_GetWindow(ui, caption);
	ui.currentWindow = &window;

	UI_PushDrawList(ui, urect{0, 0, ui.viewportSize.x, ui.viewportSize.y}, ui.fontAtlasH);

	UIDrawList &drawList = UI_GetDrawList(ui);
	drawList.sortKey.layer = window.layer;
	drawList.sortKey.order = 0;

	UI_BeginLayout(ui, UiLayoutVertical);
	UI_BeginWidget(ui, window.pos, window.size);

	if ( UI_WidgetClicked(ui) )
	{
		ui.wantsMouseInput = true;
		window.dragging = true;
	}

	UI_PushColor(ui, UiColorBorder);
	UI_AddBorder(ui, window.pos, window.size, UiBorderSize.x);
	UI_PopColor(ui);

	const float2 titlebarPos = window.pos + UiBorderSize;
	const float2 titlebarSize = float2{window.size.x - 2.0f * UiBorderSize.x, 18.0f};
	UI_PushColor(ui, UiColorCaption);
	UI_AddRectangle(ui, titlebarPos, titlebarSize);
	UI_PopColor(ui);

	const float2 panelPos = titlebarPos + float2{0.0, titlebarSize.y};
	const float2 panelSize = window.size - 2.0 * UiBorderSize - float2{ 0.0, titlebarSize.y };
	UI_PushColor(ui, UiColorPanel);
	UI_AddRectangle(ui, panelPos, panelSize);
	UI_PopColor(ui);

	const float cornerSize = 14;
	const float2 cornerBR = window.pos + window.size - UiBorderSize;
	const float2 cornerTR = cornerBR + float2{0.0, -cornerSize};
	const float2 cornerBL = cornerBR + float2{-cornerSize, 0.0};
	const float2 cornerTL = cornerBR + float2{-cornerSize, -cornerSize};
	UI_BeginWidget(ui, cornerTL, float2{cornerSize, cornerSize});
	const bool cornerHovered = UI_WidgetHovered(ui);
	UI_PushColor(ui, cornerHovered ? UiColorWidgetHover : UiColorWidget);
	UI_AddTriangle(ui, cornerBL, cornerBR, cornerTR);
	UI_PopColor(ui);
	if (UI_WidgetClicked(ui))
	{
		window.dragging = false;
		window.resizing = true;
	}
	UI_EndWidget(ui);

	const float2 captionPos = UI_AdjustTextVertically(ui, titlebarPos + float2{UiWindowPadding.x, 0.0}, titlebarSize.y);
	UI_AddText(ui, captionPos, caption);

	UI_SetCursorPos(ui, window.pos + UiWindowPadding + UiBorderSize);
	UI_MoveCursorDown(ui, titlebarSize.y);

	const uint2 contentPos = {
		.x = (u32)(panelPos.x + UiWindowPadding.x),
		.y = (u32)(panelPos.y + UiWindowPadding.y),
	};
	const uint2 contentSize = {
		.x = (u32)(panelSize.x - 2.0f * UiWindowPadding.x),
		.y = (u32)(panelSize.y - 2.0f * UiWindowPadding.y),
	};
	const urect contentRect = { .pos = contentPos, .size = contentSize };
	UI_PushDrawList(ui, contentRect, ui.fontAtlasH);
}

void UI_EndWindow(UI &ui)
{
	ASSERT(ui.currentWindow);
	ui.currentWindow = nullptr;

	UI_PopDrawList(ui);
	UI_EndWidget(ui);
	UI_EndLayout(ui);
	UI_PopDrawList(ui);
}

UIWindow &UI_GetCurrentWindow(UI &ui)
{
	ASSERT(ui.currentWindow);
	return *ui.currentWindow;
}

float2 UI_GetContentSize(const UIWindow &window)
{
	const float2 size = window.size - 2.0 * ( UiWindowPadding + UiBorderSize );
	return size;
}

void UI_Label(UI &ui, const char *text)
{
	UI_AddText(ui, ui.currentPos, text);

	const float textHeight = UI_TextHeight(ui);
	UI_MoveCursorDown(ui, textHeight + UiSpacing);
}

bool UI_Button(UI &ui, const char *text)
{
	constexpr float2 padding = {4.0f, 3.0f};
	const float2 textSize = UI_TextSize(ui, text);
	const float2 size = textSize + padding + padding;

	const float2 pos = ui.currentPos;

	UI_BeginWidget(ui, pos, size);

	UI_PushColor(ui, UI_WidgetColor(ui));
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);

	const float2 textPos = pos + padding;
	UI_AddText(ui, textPos, text);

	const bool clicked = UI_WidgetClicked(ui);
	if (clicked) {
		ui.avoidWindowInteraction = true;
	}

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, size);

	return clicked;
}

bool UI_Radio(UI &ui, const char *text, bool active)
{
	const float2 ballPos = ui.currentPos;
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
	if (clicked) {
		ui.avoidWindowInteraction = true;
	}

	UI_AddText(ui, adjustedPos, text);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, widgetSize);

	return clicked;
}

bool UI_Checkbox(UI &ui, const char *text, bool *checked)
{
	ASSERT(checked != nullptr);

	const float2 boxPos = ui.currentPos;
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
		ui.avoidWindowInteraction = true;
		*checked = !*checked;
	}

	UI_AddText(ui, adjustedPos, text);

	UI_EndWidget(ui);

	UI_CursorAdvance(ui, widgetSize);

	return clicked;
}

void UI_Separator(UI &ui)
{
	const UIWindow &window = UI_GetCurrentWindow(ui);
	const float contentWidth = UI_GetContentSize(window).x;

	const float2 pos = ui.currentPos;
	const float2 size = { contentWidth, 1.0 };

	UI_PushColor(ui, UiColorBorder);
	UI_AddRectangle(ui, pos, size);
	UI_PopColor(ui);
	UI_CursorAdvance(ui, size);
}

void UI_Image(UI &ui, ImageH image)
{
	const float2 pos = ui.currentPos;
	const float2 size = { 50.0f, 50.0f };
	const float2 uvPos = {0.0f, 0.0f};
	const float2 uvSize = {1.0f, 1.0f};
	UI_PushDrawList(ui, image);
	UI_BeginWidget(ui, pos, size);
	UI_AddQuad(ui, pos, size, uvPos, uvSize, UiColorWhite);
	UI_EndWidget(ui);
	UI_CursorAdvance(ui, size);
	UI_PopDrawList(ui);
}

// TODO: We should depend only on tools_gfx.h while this is a feature in main_gfx.cpp.
struct Graphics;
ImageH CreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels);

void UI_Initialize(UI &ui, Graphics &gfx, GraphicsDevice &gfxDev, Arena scratch)
{
	const u32 vertexBufferSize = KB(64);
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

	// Load TTF font texture
	FilePath fontPath = MakePath("assets/ProggyClean.ttf");
	DataChunk *chunk = PushFile( scratch, fontPath.str );
	if ( !chunk )
	{
		LOG(Error, "Could not open file %s\n", fontPath.str);
		return;
	}
	const byte *fontData = chunk->bytes;

	const u32 fontAtlasWidth = 128;
	const u32 fontAtlasHeight = 64;
	byte *fontAtlasBitmap = PushArray(scratch, byte, fontAtlasWidth * fontAtlasHeight);

	stbtt_fontinfo font;
	const int fontIndex = 0;
	const int fontOffset = stbtt_GetFontOffsetForIndex(fontData, fontIndex);
	const int pixelHeight = 13.0f;
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

	// Create texture
	ui.fontAtlasH = CreateImage(gfx, "texture_font", fontAtlasWidth, fontAtlasHeight, 4, false, (byte*)fontAtlasBitmapRGBA);
	ui.fontAtlasSize = {fontAtlasWidth, fontAtlasHeight};
}

void UI_BeginFrame(UI &ui)
{
	ui.frameIndex = ( ui.frameIndex + 1 ) % MAX_FRAMES_IN_FLIGHT;
	ui.vertexPtr = ui.frontendVertices;
	ui.vertexCount = 0;
	ui.drawListCount = 0;
	ui.drawListStackSize = 0;
}

void UI_EndFrame(UI &ui)
{
	// Create a new array of sorted draw lists
	u32 sortedDrawListIndices[ARRAY_COUNT(ui.drawLists)];
	for (u32 i = 0; i < ui.drawListCount; ++i)
	{
		sortedDrawListIndices[i] = i;
	}
	// Bubblesort
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
		ui.wantsMouseInput = false;
	}

	// Setup hovered / active window
	ui.hoveredWindow = nullptr;
	u32 hoveredWindowLayer = U32_MAX;
	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];

		if ( UI_MouseInArea(ui, window.pos, window.size) && window.layer < hoveredWindowLayer )
		{
			ui.hoveredWindow = &window;
			hoveredWindowLayer = window.layer;
		}
	}
	if ( UI_IsMouseClick(ui) )
	{
		if ( ui.hoveredWindow )
		{
			UI_RaiseWindow(ui, *ui.hoveredWindow);
			ui.activeWindow = ui.hoveredWindow;
		}
		else
		{
			ui.activeWindow = nullptr;
		}
	}

	for (u32 i = 0; i < ui.windowCount; ++i)
	{
		UIWindow &window = ui.windows[i];

		// In case some widget interaction blocked the window...
		if ( UI_IsMouseIdle(ui) )
		{
			window.dragging = false;
			window.resizing = false;
		}

		if ( !ui.avoidWindowInteraction )
		{
			if ( window.resizing )
			{
				constexpr float2 minWindowSize = { 100.0f, 100.0f };
				window.size = Max(minWindowSize, window.size + float2{(float)ui.input.mouse.dx, (float)ui.input.mouse.dy});
			}
			else if ( window.dragging )
			{
				window.pos += float2{(float)ui.input.mouse.dx, (float)ui.input.mouse.dy};
			}
		}
	}
}

void UI_Cleanup(const UI &ui)
{
	// Nothing to do as gfx resources are cleaned up on gfx lib shutdown
}

#endif // #ifndef TOOLS_UI_H
