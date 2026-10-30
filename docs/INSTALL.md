# 安装与部署

[返回首页](../README.md) · [使用指南](USAGE.md)

## 依赖

- C++17 编译器、CMake ≥ 3.16、Ninja
- Qt ≥ 6.2（Core、Network、Widgets；测试另需 Qt Test）
- 可选：`nmap`（Nmap 对照测试）、`desktop-file-utils`（校验桌面入口）

Ubuntu 22.04 / 24.04：

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build qt6-base-dev libgl-dev nmap desktop-file-utils
```

运行需要图形桌面（X11 或 Wayland）。扫描使用普通 TCP socket，不需要 root。

## 构建与测试

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/netscanner
```

只构建程序、不构建测试时加 `-DBUILD_TESTING=OFF`。

## 安装

安装到当前用户：

```bash
cmake --install build --prefix "$HOME/.local"
```

安装内容为 `~/.local/bin/netscanner`、应用菜单入口、SVG 图标和 AppStream 元数据。桌面入口使用 `Exec=netscanner`，桌面会话的 PATH 需要包含 `~/.local/bin`；首次安装后如菜单无法启动，注销并重新登录即可。

系统范围安装：

```bash
sudo cmake --install build --prefix /usr/local
```

原生二进制依赖本机 Qt6 动态库，跨 Ubuntu 版本分发请使用 Flatpak。

## Flatpak

清单位于 `packaging/io.github.ZiqiChai.NetScanner.yml`，使用 KDE 6.10 runtime。

```bash
sudo apt install flatpak flatpak-builder
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak-builder --user --install-deps-from=flathub --force-clean --repo=repo build-flatpak packaging/io.github.ZiqiChai.NetScanner.yml
flatpak build-bundle repo netscanner.flatpak io.github.ZiqiChai.NetScanner
flatpak install --user ./netscanner.flatpak
flatpak run io.github.ZiqiChai.NetScanner
```

沙箱权限：网络、IPC、Wayland / X11 显示和 DRI 设备，不授予主目录访问；CSV 导出通过文件选择器 portal 写入。

## GitHub Actions

`.github/workflows/build.yml` 在 push、pull request 和手动触发时运行：

1. 在 Ubuntu 22.04 与 24.04 上构建、运行 CTest、校验桌面入口。
2. 将安装目录打包为 `netscanner-ubuntu-<版本>.tar.gz` 并附 SHA256 文件上传。
3. 构建 Flatpak，上传 `netscanner.flatpak` 及其 SHA256 文件。

下载后用 `sha256sum -c` 校验。原生 tar 包是安装目录树，依赖对应 Ubuntu 版本的 Qt6 系统库。

## 常见问题

- **CMake 找不到 Qt6**：确认已安装 `qt6-base-dev` 与 `libgl-dev`（Qt6Gui 依赖 OpenGL 头文件），并检查 `CMAKE_PREFIX_PATH` 是否指向其他 Qt。
- **缺少 xcb / Wayland 平台插件**：从终端启动查看具体报错。
- **应用列表中没有图标或无法启动**：确认安装前缀与桌面会话 PATH，重新登录后再试。
- **扫描结果为空**：默认隐藏无响应主机，见[使用指南](USAGE.md#搜索隐藏与详情)。
