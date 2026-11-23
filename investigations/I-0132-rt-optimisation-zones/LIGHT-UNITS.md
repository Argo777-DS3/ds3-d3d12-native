# LIGHT-UNITS: the light sources in physical terms (I-0132 R11h, R11-PLAN §9 step 2)

Written 2026-11-09 (Paris) by the R pilot. Sources: Unreal Engine's documentation, read on 2026-11-09 (« Using Physical Lighting
Units », « Auto Exposure », UE 5.9 pages); Lagarde and de Rousiers, « Moving Frostbite to Physically Based Rendering » (SIGGRAPH 2014
course notes v3.2): **not read** (the PDF is over the 10 MB fetch limit), its figures below are **connaissance générale** until it
is read; the real-world values are connaissance générale unless a source is named. Measured DS3 values are OBSERVED with their run.

## 1. Units by light type (Unreal's page, read)

| Light | Unit | Note |
|---|---|---|
| Directional (the sun, the moon) | lux (lm/m²), « direct normal illuminance » | |
| Sky, emissive surfaces | cd/m² (luminance) | an emissive surface's value is its luminance before any lighting |
| Point, spot, rect | cd or lm (or Unreal's « unitless ») | only with the inverse-square falloff; the attenuation radius then only clamps |
| Conversions | 1 cd = 1 lm/sr; a point light Φ / 4π sr; a spot Φ / (2π (1 − cos θ)) sr; a rect Φ / 2π sr | 1 cd = 625 unitless |

Frostbite (connaissance générale): a spot's lumens divided by π (not by its cone) so a cone change keeps its brightness; a punctual
light's illuminance I / max(d², 0.01²) times the window saturate(1 − (d / r)⁴)², r the light's radius of influence.

## 2. Exposure (Unreal's page read; Frostbite connaissance générale)

- Unreal: B = Exposure × L (L the luminance in cd/m², B the pixel before the tone map); Exposure = 1 / 2^(EV100 + compensation); the
  histogram (64 bins) of log2 luminance, the average between a low and a high percentile (« good starting ranges » 70-80 and 80-95)
  taken as 18 % middle grey; speed up and down in stops a second, linear far from the target and exponential within 1.5 stops (a
  continuous first derivative); an exposure compensation curve by the scene's average EV100; pre-exposure (the previous frame's
  exposure applied before the scene colour is written) against low-precision overflow.
- Frostbite (connaissance générale): EV100 = log2(L_avg × 100 / K), K = 12.5; the saturation-based exposure 1 / (1.2 × 2^EV100).

R11h's pass (step 1) takes Unreal's law and its percentile trimming (10th to 90th: a mean level, not the highlights), with one target
for the whole game (one universal rule): the original's mean displayed level, measured on its pictures (§4).

## 3. Real values (connaissance générale)

| Source | Value |
|---|---|
| The sun, clear noon | 100,000 to 120,000 lux |
| The sun low (sunrise, sunset) | about 10,000 to 30,000 lux (strongly variable) |
| Overcast daylight | 1,000 to 10,000 lux |
| The clear sky's luminance | about 5,000 to 10,000 cd/m² (brighter at the horizon) |
| The full moon | 0.1 to 0.3 lux |
| A candle | about 12.6 lm, 1 cd; its flame about 10,000 cd/m² |
| An oil lamp, a torch | about 100 to 400 lm (8 to 30 cd), uncertain |
| A campfire, a brazier | thousands of lumens (2,000 to 10,000), very uncertain |
| Lava | glowing at about 1,000 to 1,200 °C: a luminance of thousands of cd/m², uncertain |

## 4. DS3's values (OBSERVED)

- **Local lights** (Firelink, `i132-r11d2-m40-gi-1920x1080`, rows `native_light_near`; the shader's colour is the param's colour² ×
  strength × scale, `07_scene_lighting.inc`): the common torch-like light (0.47, 0.31, 0.20) × 1, radius 2 m: a luminance-weighted
  intensity about 0.12 (DS3's units: the illuminance at 1 m); the bonfire's (1.0, 0.51, 0.39) × 1.2, radius 6 m: about 0.6; a blue
  light (0.59, 0.76, 1.0) × 1.4, radius 12 m; a pale one (0.88, 1, 0.96) × 0.3, radius 2.5 m. R11d's default doubles them (x2).
  DS3's falloff max(1 / d² − 1 / R², 0): the inverse square with a cut at R.
- **The daylight** (the High Wall at sunset, `i132-r11d1-m30-ptsrc-1920x1080/world-1500-hdr.rgba16f`, before the tone map's
  exposure 1.52): the sky's luminance about 0.02 to 0.07 (the top rows' median and 95th percentile), the brightest surfaces about
  0.05 to 0.2; so DS3's sun gives about 1 unit of illuminance (a lit surface of albedo 0.3 at about 0.08), and its sky is about as
  bright as its sunlit surfaces, as outdoors (INFERRED from region statistics, no depth or normal used).
- **The anchor** (INFERRED, from the values above and §3's connaissance générale): if DS3's sun of about 1 unit stands for a real sun of 10,000 to 100,000 lux, one DS3 unit is 10,000 to 100,000
  lux, and a real torch of 8 to 30 cd is 1e-4 to 3e-3 units at 1 m against DS3's 0.12 to 0.24: DS3's local lights are 100 to 1,000
  times too strong against its sun, the flames themselves (the effects' colour, about 1 unit) about right (a flame's 10,000 cd/m²).

## 5. The test tools (R11h step 2, `rt-local-physical-10/-50/-200.flag`)

One rule for every local light (rt_scene.h RtPhysicalLight): the intensity at 1 m from DS3's colours (a point's as they are, DS3's
diffuse falloff max(1 / d² − 1 / R², 0) being the inverse square at 1 m; a legacy point's and a spot's without its ShadingMode, whose
falloffs are not the inverse square, matched to it at half their range: x R² / 16), times the ratio to the sun (0.1, 0.02, 0.005
against DS3's x1); the falloff 1 / max(d², 1 cm²) windowed by saturate(1 − (d / r)⁴)², r = sqrt(L / 0.0019), L the luminance of the
brighter of its diffuse and specular before the ratio (DS3's torch 8 m, the bonfire 18 m); the soft shadow's sphere and the hits'
stop keep DS3's range (the light's size unchanged).

## 6. What follows (to the sheets, Thomas choosing)

- The daylight (the sun, the sky, the moon) kept in DS3's units as the reference (its colour by area is the art's time of day).
- The local lights at their physical ratio to the sun by family (D-0150; the families to be named from what the collect sees), with
  the inverse-square falloff and Frostbite's window instead of DS3's cut; their flames (the effects) unchanged.
- The automatic exposure on, so the interiors and the night are lifted as the eye lifts them.
- fp16 headroom: a torch-lit wall at 1e-3 units and a dark corner at 1e-5 stay representable in R16G16B16A16 (subnormal below
  6e-5), coarse in R11G11B10 (its targets to list before the sheets); pre-exposure if they band.
