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
| Primary (CarWash Blue) | ![#3399EE](https://via.placeholder.com/15/3399EE/3399EE) | `#3399EE` | `51, 153, 238` |
| Secondary (Water) | ![#66CCFF](https://via.placeholder.com/15/66CCFF/66CCFF) | `#66CCFF` | `102, 204, 255` |
| Accent (Soap) | ![#FFFFFF](https://via.placeholder.com/15/FFFFFF/FFFFFF) | `#FFFFFF` | `255, 255, 255` |
| Background | ![#1A1A2E](https://via.placeholder.com/15/1A1A2E/1A1A2E) | `#1A1A2E` | `26, 26, 46` |

*Note: These match the rasterizer's shading colors in rasterizer.cpp:350-354*

## Icon Concept

```
    ┌────────────────┐
    │    ╱╲    ≋≋≋   │   Water droplet + waves
    │   ╱  ╲   ≋≋    │   Represents: AI "washing" the render
    │  ╱    ╲  ≋     │
    │  ╲    ╱        │   Blue gradient: #3399EE → #66CCFF
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
