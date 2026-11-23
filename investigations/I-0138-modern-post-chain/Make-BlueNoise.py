"""I-0138 tranche 5: the output dither's blue-noise texture, made here (no third-party texture, so no licence to carry).

The void-and-cluster method (R. Ulichney, « The void-and-cluster method for dither array generation », 1993), on a torus:
a Gaussian energy around every set point; an initial pattern relaxed until its tightest cluster and its largest void are one
place; then every texel ranked, the initial points by removing the tightest cluster, the rest by filling the largest void (past
one half, the tightest cluster of the points still unset). The ranks, 0 to N*N - 1, are the thresholds: scaled to 8 bits, every
value N*N/256 times.

    python -I Make-BlueNoise.py <out.h> [--size 64] [--seed 5138] [--sigma 1.5]

Writes a C++ header (`inline constexpr std::array<uint8_t, N*N> kBlueNoise`, rows top to bottom) and prints a check: the
texture's power spectrum averaged by radius, as a fraction of its variance (a white noise: 1 at every radius; a blue noise:
about 0 at the low frequencies, its energy past half the Nyquist radius). Deterministic for a seed. Standard library only; a
few seconds at 64.
"""
import argparse, cmath, math, random, sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('out')
    parser.add_argument('--size', type=int, default=64)
    parser.add_argument('--seed', type=int, default=5138)
    parser.add_argument('--sigma', type=float, default=1.5)
    args = parser.parse_args()
    n = args.size
    count = n * n
    if count % 256:
        sys.exit('the size squared must be a multiple of 256')
    radius = int(math.ceil(4.0 * args.sigma))
    kernel = [(dx, dy, math.exp(-(dx * dx + dy * dy) / (2.0 * args.sigma * args.sigma)))
              for dy in range(-radius, radius + 1) for dx in range(-radius, radius + 1)]
    # For each texel, the indices its kernel touches (the torus), once.
    offsets = [(dx, dy) for dx, dy, _ in kernel]
    weights = [w for _, _, w in kernel]

    def touched(index):
        x, y = index % n, index // n
        return [((y + dy) % n) * n + (x + dx) % n for dx, dy in offsets]

    def add(energy, index, sign):
        for at, w in zip(touched(index), weights):
            energy[at] += sign * w

    rng = random.Random(args.seed)
    ones = [False] * count
    energy = [0.0] * count          # the energy of the set points
    for index in rng.sample(range(count), count // 10):
        ones[index] = True
        add(energy, index, 1.0)

    def tightest(state, field):     # the set point with the most energy
        return max((i for i in range(count) if state[i]), key=field.__getitem__)

    def largest_void(state, field):  # the unset point with the least energy
        return min((i for i in range(count) if not state[i]), key=field.__getitem__)

    while True:                     # relax the initial pattern
        cluster = tightest(ones, energy)
        ones[cluster] = False
        add(energy, cluster, -1.0)
        void = largest_void(ones, energy)
        ones[void] = True
        add(energy, void, 1.0)
        if void == cluster:
            break
    rank = [0] * count
    initial = sum(ones)
    # Phase 1: the initial points, from the last rank down, by removing the tightest cluster.
    state, field = list(ones), list(energy)
    for r in range(initial - 1, -1, -1):
        cluster = tightest(state, field)
        state[cluster] = False
        add(field, cluster, -1.0)
        rank[cluster] = r
    # Phase 2: up to one half, by filling the largest void.
    state, field = list(ones), list(energy)
    r = initial
    while r < count // 2:
        void = largest_void(state, field)
        state[void] = True
        add(field, void, 1.0)
        rank[void] = r
        r += 1
    # Phase 3: the unset points are now the minority: the tightest cluster of them is set next.
    unset = [not s for s in state]
    field = [0.0] * count
    for index in range(count):
        if unset[index]:
            add(field, index, 1.0)
    while r < count:
        cluster = tightest(unset, field)
        unset[cluster] = False
        add(field, cluster, -1.0)
        rank[cluster] = r
        r += 1
    if sorted(rank) != list(range(count)):
        sys.exit('the ranks are not a permutation')
    values = [(v * 256) // count for v in rank]

    # The check: the power by radius (a separable DFT), over the variance.
    mean = sum(values) / count
    centred = [v - mean for v in values]
    variance = sum(v * v for v in centred) / count
    wave = [cmath.exp(-2j * math.pi * k / n) for k in range(n)]
    rows = [[sum(centred[y * n + x] * wave[(x * u) % n] for x in range(n)) for u in range(n)] for y in range(n)]
    bins = {}
    for q in range(n):
        for u in range(n):
            radius_bin = int(round(math.hypot(min(u, n - u), min(q, n - q))))
            if 0 < radius_bin <= n // 2:
                power = abs(sum(rows[y][u] * wave[(y * q) % n] for y in range(n))) ** 2 / count
                bins.setdefault(radius_bin, []).append(power / variance)
    radial = {k: sum(v) / len(v) for k, v in bins.items()}
    low = max(radial[k] for k in radial if k <= n // 8)
    print(f'{n}x{n}, seed {args.seed}, sigma {args.sigma}: power over variance by radius (Nyquist {n // 2}): ' +
          ' '.join(f'{k}:{radial[k]:.2f}' for k in sorted(radial) if k % 4 == 0 or k == 1) + f'; at most {low:.3f} up to radius {n // 8}')
    if low > 0.05:
        sys.exit('not a blue noise: energy at the low frequencies')
    with open(args.out, 'w', encoding='ascii', newline='\n') as out:
        out.write('// Written by investigations/I-0138-modern-post-chain/Make-BlueNoise.py (I-0138 tranche 5): the output dither\'s blue noise,\n')
        out.write(f'// void and cluster (Ulichney 1993) on a {n}x{n} torus, seed {args.seed}, sigma {args.sigma}; thresholds 0 to 255, each {count // 256} times; rows top to bottom.\n')
        out.write('#pragma once\n#include <array>\n#include <cstdint>\n\nnamespace ds3native {\n')
        out.write(f'inline constexpr uint32_t kBlueNoiseEdge={n};\ninline constexpr std::array<uint8_t,{count}> kBlueNoise{{{{\n')
        for y in range(n):
            out.write('    ' + ','.join(str(values[y * n + x]) for x in range(n)) + ',\n')
        out.write('}};\n}\n')


if __name__ == '__main__':
    main()
