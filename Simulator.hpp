#pragma once

#include <iostream>
#include <random>
#include <cassert>
#include <fstream>

constexpr size_t t = 5'000;
constexpr size_t save_rate = 100;
constexpr std::array<std::pair<int, int>, 4> deltas{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
inline std::mt19937_64 rnd(1337);

constexpr std::size_t mx_size = std::numeric_limits<std::size_t>::max();

template <typename type_p, typename type_v, typename type_vf, size_t Rows, size_t Columns>
class Simulator {
public:
    static constexpr bool is_static = Rows != mx_size && Columns != mx_size;

    template <class Type>
    using ArrayType = std::conditional_t<is_static,
            std::array<std::array<Type, Columns>, Rows>,
            std::vector<std::vector<Type>>>;

    using Field = ArrayType<char>;
    Field field{};

    using IntArray = ArrayType<int>;
    type_p rho[256];
    size_t N, M;

    ArrayType<type_p> p{}, old_p{};
    IntArray dirs{}, last_use{};

    int UT = 0;
    type_v g;

    template <typename type_cur>
    struct VectorField {
        std::vector<std::vector<std::array<type_cur, deltas.size()>>> v;
        VectorField(size_t n, size_t m) {
            v.resize(n, std::vector<std::array<type_cur, deltas.size()>>(m));
        }
        VectorField() = default;
        void F(size_t n, size_t m) {
            v.resize(n, std::vector<std::array<type_cur, deltas.size()>>(m));
        }

        type_cur &Add(int x, int y, int dx, int dy, type_cur dv) {
            return Get(x, y, dx, dy) += dv;
        }

        type_cur &Get(int x, int y, int dx, int dy) {
            size_t i = std::ranges::find(deltas, std::pair(dx, dy)) - deltas.begin();
            assert(i < deltas.size());
            return v[x][y][i];
        }
    };

    struct ParticleParams {
        char type;
        type_p cur_p;
        std::array<type_v, deltas.size()> v;

        void SwapWith(int x, int y, VectorField<type_v>& velocity1, ArrayType<type_p> &p1, Field &field1) {
            std::swap(field1[x][y], type);
            std::swap(p1[x][y], cur_p);
            std::swap(velocity1.v[x][y], v);
        }
    };

    VectorField<type_v> velocity{};
    VectorField<type_vf> velocity_flow{};

    explicit constexpr Simulator(const std::vector<std::vector<char>>& field1, float rho_air, int rho_fluid, float g1) noexcept
    requires(is_static) : g(g1) {
        std::cout << "Static version constructor called with sizes: " << Rows << " " << Columns - 1 << "\n";
        N = field1.size();
        M = field1.front().size() - 1;
        velocity.F(N, M);
        velocity_flow.F(N, M);
        for (std::size_t i = 0; auto& row : field) {
            const std::vector<char>& dynamic_field = field1[i];
            assert(dynamic_field.size() == row.size());
            std::ranges::copy(dynamic_field, row.begin());
            i++;
        }
        rho[' '] = rho_air;
        rho['.'] = rho_fluid;
    }

    explicit constexpr Simulator(const Field& field1, float rho_air, int rho_fluid, float g1) requires(!is_static) : Simulator(field1, rho_air, rho_fluid, g1, field1.size(), field1.front().size()) {};

    explicit constexpr Simulator(const Field& field1, float rho_air, int rho_fluid, float g1, size_t rows, size_t cols) requires(!is_static) : field(field1), p{rows, typename ArrayType<type_p>::value_type(cols)},
                                                                                                                                                 old_p{rows, typename ArrayType<type_p>::value_type(cols)},
                                                                                                                                                 dirs{rows, typename IntArray::value_type(cols)},
                                                                                                                                                 last_use{rows, typename IntArray::value_type(cols)},
                                                                                                                                                 g(g1) {
        std::cout << "Dynamic version with sizes:" << field1.size() << " " << field1.front().size() - 1 << "\n";
        rho[' '] = rho_air;
        rho['.'] = rho_fluid;
        N = field1.size();
        M = field1.front().size() - 1;
        velocity.f(N, M);
        velocity_flow.f(N, M);
    }

    void SaveToFile() {
        std::ofstream f_out;
        f_out.open("dump.txt");
        if (!f_out) {
            std::cerr << "Error during opening file\n";
            exit(0);
        }

        f_out << N << " " << M << "\n";
        for (const auto& row : field) {
            for (const char ch : row) {
                if (ch != '\0') {
                    f_out << ch;
                }
            }
            f_out << "\n";
        }

        f_out << rho[' '] << "\n" << rho['.'] << "\n" << g << "\n";
    }

    type_p Random01() {
        if constexpr (std::is_same_v<type_p, float>) {
            return static_cast<type_p>(static_cast<float>(rnd()) / static_cast<float>(std::mt19937_64::max()));
        } else if constexpr (std::is_same_v<type_p, double>) {
            return static_cast<type_p>(static_cast<double>(rnd()) / static_cast<double>(std::mt19937_64::max()));
        } else {
            return type_p::from_raw(rnd() & (1 << type_p::getK()) - 1);
        }

    }

    void PropagateStop(int x, int y, bool force = false) {
        if (!force) {
            bool stop = true;
            for (auto [dx, dy] : deltas) {
                int nx = x + dx, ny = y + dy;
                if (field[nx][ny] != '#' && last_use[nx][ny] < UT - 1 && velocity.Get(x, y, dx, dy) > static_cast<type_v>(0)) {
                    stop = false;
                    break;
                }
            }
            if (!stop) {
                return;
            }
        }
        last_use[x][y] = UT;
        for (auto [dx, dy] : deltas) {
            int nx = x + dx, ny = y + dy;
            if (field[nx][ny] == '#' || last_use[nx][ny] == UT || velocity.Get(x, y, dx, dy) > static_cast<type_v>(0)) {
                continue;
            }
            PropagateStop(nx, ny);
        }
    }

    type_p MoveProb(int x, int y) {
        type_p sum = 0;
        for (auto [dx, dy] : deltas) {
            int nx = x + dx, ny = y + dy;
            if (field[nx][ny] == '#' || last_use[nx][ny] == UT) {
                continue;
            }
            auto v = velocity.Get(x, y, dx, dy);
            if (v < static_cast<type_v>(0)) {
                continue;
            }
            sum += static_cast<type_p>(v);
        }
        return sum;
    }

    bool PropagateMove(int x, int y, bool is_first) {
        last_use[x][y] = UT - is_first;
        bool ret = false;
        int nx = -1, ny = -1;
        do {
            std::array<type_p, deltas.size()> tres;
            type_p sum = 0;
            for (size_t i = 0; i < deltas.size(); ++i) {
                auto [dx, dy] = deltas[i];
                int nx1 = x + dx, ny1 = y + dy;
                if (field[nx1][ny1] == '#' || last_use[nx1][ny1] == UT) {
                    tres[i] = sum;
                    continue;
                }
                auto v = velocity.Get(x, y, dx, dy);
                if (v < static_cast<type_v>(0)) {
                    tres[i] = sum;
                    continue;
                }
                sum += static_cast<type_p>(v);
                tres[i] = sum;
            }

            if (sum == static_cast<type_p>(0)) {
                break;
            }

            auto p = static_cast<type_p>(Random01() * sum);
            size_t d = std::ranges::upper_bound(tres, p) - tres.begin();

            auto [dx, dy] = deltas[d];
            nx = x + dx;
            ny = y + dy;
            assert(velocity.Get(x, y, dx, dy) > static_cast<type_v>(0) && field[nx][ny] != '#' && last_use[nx][ny] < UT);

            ret = (last_use[nx][ny] == UT - 1 || PropagateMove(nx, ny, false));
        } while (!ret);
        last_use[x][y] = UT;
        for (auto [dx, dy] : deltas) {
            int nx1 = x + dx, ny1 = y + dy;
            if (field[nx1][ny1] != '#' && last_use[nx1][ny1] < UT - 1 && velocity.Get(x, y, dx, dy) < static_cast<type_v>(0)) {
                PropagateStop(nx1, ny1);
            }
        }
        if (ret) {
            if (!is_first) {
                ParticleParams pp{};
                pp.SwapWith(x, y, velocity, p, field);
                pp.SwapWith(nx, ny, velocity, p, field);
                pp.SwapWith(x, y, velocity, p, field);
            }
        }
        return ret;
    }

    std::tuple<type_p, bool, std::pair<int, int>> PropagateFlow(int x, int y, type_p lim) {
        last_use[x][y] = UT - 1;
        type_p ret = 0;
        for (auto [dx, dy] : deltas) {
            int nx = x + dx, ny = y + dy;
            if (field[nx][ny] != '#' && last_use[nx][ny] < UT) {
                auto cap = velocity.Get(x, y, dx, dy);
                auto flow = velocity_flow.Get(x, y, dx, dy);
                if (flow == static_cast<type_vf>(cap)) {
                    continue;
                }
                type_v res = cap - static_cast<type_v>(flow);
                auto vp = std::min(lim, static_cast<type_p>(res));
                if (last_use[nx][ny] == UT - 1) {
                    velocity_flow.Add(x, y, dx, dy, static_cast<type_vf>(vp));
                    last_use[x][y] = UT;
                    return {vp, 1, {nx, ny}};
                }
                auto [t, prop, end] = PropagateFlow(nx, ny, vp);
                ret += t;
                if (prop) {
                    velocity_flow.Add(x, y, dx, dy, static_cast<type_vf>(t));
                    last_use[x][y] = UT;
                    return {t, end != std::pair(x, y), end};
                }
            }
        }
        last_use[x][y] = UT;
        return {ret, 0, {0, 0}};
    }


    void Run() {
        for (size_t x = 0; x < N; ++x) {
            for (size_t y = 0; y < M; ++y) {
                if (field[x][y] == '#') {
                    continue;
                }
                for (auto [dx, dy] : deltas) {
                    dirs[x][y] += field[x + dx][y + dy] != '#';
                }
            }
        }

        for (size_t i = 0; i < t; ++i) {

            type_p total_delta_p = 0;
            for (size_t x = 0; x < N; ++x) {
                for (size_t y = 0; y < M; ++y) {
                    if (field[x][y] == '#') {
                        continue;
                    }
                    if (field[x + 1][y] != '#') {
                        velocity.Add(x, y, 1, 0, g);
                    }
                }
            }

            for (int i1 = 0; i1 < p.size(); ++i1) {
                for (int j1 = 0; j1 < p.front().size(); ++j1) {
                    old_p[i1][j1] = p[i1][j1];
                }
            }
            for (size_t x = 0; x < N; ++x) {
                for (size_t y = 0; y < M; ++y) {
                    if (field[x][y] == '#') {
                        continue;
                    }
                    for (auto [dx, dy] : deltas) {
                        int nx = static_cast<int>(x) + dx, ny = static_cast<int>(y) + dy;
                        if (field[nx][ny] != '#' && old_p[nx][ny] < old_p[x][y]) {
                            auto delta_p = old_p[x][y] - old_p[nx][ny];
                            auto force = delta_p;
                            auto &contr = velocity.Get(nx, ny, -dx, -dy);
                            if (static_cast<type_p>(contr) * rho[static_cast<int>(field[nx][ny])] >= force) {
                                contr -= static_cast<type_v>(force / rho[static_cast<int>(field[nx][ny])]);
                                continue;
                            }
                            force -= static_cast<type_p>(contr) * rho[static_cast<int>(field[nx][ny])];
                            contr = 0;
                            velocity.Add(x, y, dx, dy, static_cast<type_v>(force / rho[static_cast<int>(field[x][y])]));
                            p[x][y] -= force / static_cast<type_p>(dirs[x][y]);
                            total_delta_p -= force / static_cast<type_p>(dirs[x][y]);
                        }
                    }
                }
            }

            velocity_flow = {N, M};
            bool prop = false;
            do {
                UT += 2;
                prop = false;
                for (size_t x = 0; x < N; ++x) {
                    for (size_t y = 0; y < M; ++y) {
                        if (field[x][y] != '#' && last_use[x][y] != UT) {
                            auto [t, local_prop, _] = PropagateFlow(x, y, 1);
                            if (t > static_cast<type_p>(0)) {
                                prop = true;
                            }
                        }
                    }
                }
            } while (prop);

            for (size_t x = 0; x < N; ++x) {
                for (size_t y = 0; y < M; ++y) {
                    if (field[x][y] == '#') {
                        continue;
                    }
                    for (auto [dx, dy] : deltas) {
                        auto old_v = velocity.Get(x, y, dx, dy);
                        auto new_v = velocity_flow.Get(x, y, dx, dy);
                        if (old_v > static_cast<type_v>(0)) {
                            assert(new_v <= static_cast<type_vf>(old_v));
                            velocity.Get(x, y, dx, dy) = static_cast<type_v>(new_v);
                            auto force = static_cast<type_p>((old_v - static_cast<type_v>(new_v))) * rho[static_cast<int>(field[x][y])];
                            if (field[x][y] == '.') {
                                force *= static_cast<type_p>(0.8);
                            }
                            if (field[x + dx][y + dy] == '#') {
                                p[x][y] += force / static_cast<type_p>(dirs[x][y]);
                                total_delta_p += force / static_cast<type_p>(dirs[x][y]);
                            } else {
                                p[x + dx][y + dy] += force / static_cast<type_p>(dirs[x + dx][y + dy]);
                                total_delta_p += force / static_cast<type_p>(dirs[x + dx][y + dy]);
                            }
                        }
                    }
                }
            }

            UT += 2;
            prop = false;
            for (size_t x = 0; x < N; ++x) {
                for (size_t y = 0; y < M; ++y) {
                    if (field[x][y] != '#' && last_use[x][y] != UT) {
                        if (Random01() < MoveProb(x, y)) {
                            prop = true;
                            PropagateMove(x, y, true);
                        } else {
                            PropagateStop(x, y, true);
                        }
                    }
                }
            }

            if (prop) {
                std::cout << "Tick " << i << ":\n";
                for (size_t x = 0; x < N; ++x) {
                    for (size_t y = 0; y < M; ++y) {
                        std::cout << static_cast<char>(field[x][y]);
                    }
                    std::cout << "\n";
                }
            }

            if (i % save_rate == 0) {
                SaveToFile();
            }
        }
    }
};
