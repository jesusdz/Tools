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


float2 operator+(float2 a, float2 b)
{
	const float2 res = { .x = a.x + b.x, .y = a.y + b.y };
	return res;
}

float2 operator-(float2 a, float2 b)
{
	const float2 res = { .x = a.x - b.x, .y = a.y - b.y };
	return res;
}

float2 operator*(float a, float2 b)
{
	const float2 res = { .x = a * b.x, .y = a * b.y };
	return res;
}

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
	const char *caption;
	float2 pos;
	float2 size;
};

struct UIWidget
{
	float2 pos;
	float2 size;
};

struct UI
{
	BufferH vertexBuffer[MAX_FRAMES_IN_FLIGHT];
	UIVertex *vertexData[MAX_FRAMES_IN_FLIGHT];
	u32 frameIndex;
	u32 vertexCount;
	u32 vertexCountLimit;

	ImageH fontAtlasH;
	float2 fontAtlasSize;
	float fontScale, fontAscent, fontDescent, fontLineGap;

	float2 whitePixelUv;

	stbtt_packedchar charData[255];

	float4 colors[16];
	u32 colorCount;

	float2 currentPos;

	UIInput input;

	UIWindow windows[16];
	bool windowBegan;

	UIWidget widgetStack[16];
	u32 widgetStackSize;
};

void UI_NextFrame(UI &ui)
{
	ui.frameIndex = ( ui.frameIndex + 1 ) % MAX_FRAMES_IN_FLIGHT;
	ui.vertexCount = 0;

	ui.currentPos = float2{10, 10};
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
}

void UI_EndWidget(UI &ui)
{
	ASSERT(ui.widgetStackSize > 0);
	ui.widgetStackSize--;
}

bool UI_IsHover(const UI &ui)
{
	bool hover = false;

	if (ui.widgetStackSize > 0)
	{
		const UIWidget widget = ui.widgetStack[ui.widgetStackSize-1];

		hover =
			ui.input.mouse.x >= widget.pos.x &&
			ui.input.mouse.x <  widget.pos.x + widget.size.x &&
			ui.input.mouse.y >= widget.pos.y &&
			ui.input.mouse.y <  widget.pos.y + widget.size.y;
	}

	return hover;
}

bool UI_IsMouseClick(const UI &ui)
{
	const bool click = ui.input.mouse.buttons[MOUSE_BUTTON_LEFT] == BUTTON_STATE_PRESS;
	return click;
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

void UI_AddQuad(UI &ui, float2 pos, float2 size, float2 uv, float2 uvSize, float4 fcolor)
{
	ASSERT(ui.vertexCount + 6 <= ui.vertexCountLimit);

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

	// add vertices
	UIVertex *vertexPtr = ui.vertexData[ui.frameIndex] + ui.vertexCount;
	*vertexPtr++ = UIVertex{ v0, uvTL, color };
	*vertexPtr++ = UIVertex{ v1, uvBL, color };
	*vertexPtr++ = UIVertex{ v2, uvBR, color };
	*vertexPtr++ = UIVertex{ v0, uvTL, color };
	*vertexPtr++ = UIVertex{ v2, uvBR, color };
	*vertexPtr++ = UIVertex{ v3, uvTR, color };

	ui.vertexCount += 6;
}

void UI_AddRectangle(UI &ui, float2 pos, float2 size)
{
	ASSERT(ui.vertexCount + 6 <= ui.vertexCountLimit);

	const float2 uvSize = {0, 0};
	const float4 color = UI_GetColor(ui);
	UI_AddQuad(ui, pos, size, ui.whitePixelUv, uvSize, color);
}

void UI_AddBorder(UI &ui, float2 pos, float2 size, float borderSize)
{
	ASSERT(ui.vertexCount + 6 <= ui.vertexCountLimit);

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

float2 UI_CalcTextSize(UI &ui, const char *text)
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

	const float textHeight = ui.fontAscent - ui.fontDescent;
	const float2 textSize = { textWidth , textHeight };
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
		const float4 charColor = {1.0, 1.0, 1.0, 1.0};

		UI_AddQuad(ui, charPos, charSize, charUv, charUvSize, charColor);

		cursorx = Round( cursorx + pc.xadvance );
		ptr++;
	}
}

UIWindow &UI_GetWindow(UI &ui, const char *caption)
{
	for (u32 i = 0; i < ARRAY_COUNT(ui.windows); ++i)
	{
		UIWindow &window = ui.windows[i];
		if (!window.caption)
		{
			window.caption = caption;
			window.pos = {100.0f, 100.0f};
			window.size = {200.0f, 300.0f};
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
	ASSERT(!ui.windowBegan);
	ui.windowBegan = true;

	UIWindow &window = UI_GetWindow(ui, caption);

	const float4 borderColor = {0.3, 0.3, 0.3, 0.9};
	const float2 borderSize = float2{1.0f, 1.0f};
	UI_PushColor(ui, borderColor);
	UI_AddBorder(ui, window.pos, window.size, borderSize.x);
	UI_PopColor(ui);

	const float4 titlebarColor = { 0.1, 0.2, 0.4, 0.8 };
	const float2 titlebarPos = window.pos + borderSize;
	const float2 titlebarSize = float2{window.size.x - 2.0f * borderSize.x, 18.0f};
	UI_PushColor(ui, titlebarColor);
	UI_AddRectangle(ui, titlebarPos, titlebarSize);
	UI_PopColor(ui);

	const float4 panelColor = {0.05, 0.05, 0.05, 0.9};
	const float2 panelPos = titlebarPos + float2{0.0, titlebarSize.y};
	const float2 panelSize = window.size - 2.0 * borderSize - float2{ 0.0, titlebarSize.y };
	UI_PushColor(ui, panelColor);
	UI_AddRectangle(ui, panelPos, panelSize);
	UI_PopColor(ui);

	const float2 windowPadding = {4.0f, 4.0f};

	const float2 captionPos = window.pos + borderSize + windowPadding;
	UI_AddText(ui, captionPos, caption);

	ui.currentPos = window.pos + windowPadding + borderSize + float2{0.0, titlebarSize.y};
}

void UI_EndWindow(UI &ui)
{
	ASSERT(ui.windowBegan);
	ui.windowBegan = false;
}

void UI_Label(UI &ui, const char *text)
{
	UI_AddText(ui, ui.currentPos, text);

	constexpr float spacing = 4.0f;
	const float2 textSize = UI_CalcTextSize(ui, text);
	const float displacement = textSize.y + spacing;
	ui.currentPos.y += displacement;
}

bool UI_Button(UI &ui, const char *text)
{
	constexpr float2 padding = {4.0f, 3.0f};
	constexpr float spacing = 4.0f;
	const float2 textSize = UI_CalcTextSize(ui, text);
	const float2 size = textSize + padding + padding;

	const float2 pos = ui.currentPos;

	UI_BeginWidget(ui, pos, size);

	const bool hover = UI_IsHover(ui);
	if (hover)
	{
		const float4 hoverColor = { 0.2, 0.4, 0.8, 1.0 };
		UI_PushColor(ui, hoverColor);
	}

	UI_AddRectangle(ui, pos, size);

	const float2 textPos = pos + padding;
	UI_AddText(ui, textPos, text);

	if (hover)
	{
		UI_PopColor(ui);
	}

	const float displacement = size.y + spacing;
	ui.currentPos.y += displacement;

	UI_EndWidget(ui);

	const bool clicked = hover && UI_IsMouseClick(ui);
	return clicked;
}

// TODO: We should depend only on tools_gfx.h while this is a feature in main_gfx.cpp.
struct Graphics;
ImageH CreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels);

void UI_Initialize(UI &ui, Graphics &gfx, GraphicsDevice &gfxDev, Arena scratch)
{
	const u32 vertexBufferSize = KB(16);
	ui.vertexCountLimit = vertexBufferSize / sizeof(UIVertex);

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		ui.vertexBuffer[i] = CreateBuffer(
			gfxDev,
			vertexBufferSize,
			BufferUsageVertexBuffer,
			HeapType_Dynamic);

		ui.vertexData[i] = (UIVertex*)GetBufferPtr(gfxDev, ui.vertexBuffer[i]);
	}

	const float4 defaultColor = { 0.1, 0.2, 0.4, 0.8 };
	UI_PushColor(ui, defaultColor);

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

void UI_Cleanup(const UI &ui)
{
	// Nothing to do as gfx resources are cleaned up on gfx lib shutdown
}

#endif // #ifndef TOOLS_UI_H
