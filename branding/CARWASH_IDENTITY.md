# HdCarWash Visual Identity

## ASCII Logo (for terminals/batch files)

```
    ╔═══════════════════════════════════════════╗
    ║   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   ║
    ║   ░  ██╗  ██╗██████╗  ░░░░░░░░░░░░░░░░   ║
    ║   ░  ██║  ██║██╔══██╗ ░░░ CARWASH ░░░░   ║
    ║   ░  ███████║██║  ██║ ░░░░░░░░░░░░░░░░   ║
    ║   ░  ██╔══██║██║  ██║ ░░ AI Render ░░░   ║
    ║   ░  ██║  ██║██████╔╝ ░░░░░░░░░░░░░░░░   ║
    ║   ░  ╚═╝  ╚═╝╚═════╝  ░░░░░░░░░░░░░░░░   ║
    ║   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   ║
    ╚═══════════════════════════════════════════╝
```

## Compact ASCII (for batch files)

```
    ╭─────────────────────────────────╮
    │  ≋≋≋ HdCarWash ≋≋≋             │
    │      AI Render Delegate         │
    │  ≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋   │
    ╰─────────────────────────────────╯
```

## Color Palette

| Use | Color | Hex | RGB |
|-----|-------|-----|-----|
| Primary (CarWash Blue) | ![#3399E6](https://via.placeholder.com/15/3399E6/3399E6) | `#3399E6` | `51, 153, 230` |
| Secondary (Water) | ![#66CCFF](https://via.placeholder.com/15/66CCFF/66CCFF) | `#66CCFF` | `102, 204, 255` |
| Accent (Soap) | ![#FFFFFF](https://via.placeholder.com/15/FFFFFF/FFFFFF) | `#FFFFFF` | `255, 255, 255` |
| Background | ![#1A1A2E](https://via.placeholder.com/15/1A1A2E/1A1A2E) | `#1A1A2E` | `26, 26, 46` |

*Note: The Primary (CarWash Blue) matches the rasterizer's base shading tint
`GfVec4f(0.2, 0.6, 0.9, 1.0)` (= `#3399E6`, RGB `51, 153, 230`) in `rasterizer.cpp`
— search for the "CarWash blue tint" comment in the shading path (cited by symbol
rather than line number, which drifts as the file changes). The other palette
colors are brand-only and are not currently used by the rasterizer.*

## Icon Concept

```
    ┌────────────────┐
    │    ╱╲    ≋≋≋   │   Water droplet + waves
    │   ╱  ╲   ≋≋    │   Represents: AI "washing" the render
    │  ╱    ╲  ≋     │
    │  ╲    ╱        │   Blue gradient: #3399E6 → #66CCFF
    │   ╲  ╱         │
    │    ╲╱          │
    └────────────────┘
```

## Icon Creation Options

1. **Quick (free)**: https://www.favicon.io/emoji-favicons/ → Search "droplet" 💧
2. **Custom**: https://www.canva.com/create/icons/ → Blue droplet with "HD" text
3. **AI Generated**: Use DALL-E/Midjourney: "minimalist blue water droplet icon, tech style, flat design"

## To Apply Icon to Batch File

Windows batch files can't have embedded icons, but you can:

1. **Create a shortcut** (.lnk) to the .bat file
2. **Right-click shortcut** → Properties → Change Icon
3. **Browse to your .ico file**

Or convert the batch to an executable with icon using:
- `bat2exe` tool
- `iexpress` (built into Windows)
