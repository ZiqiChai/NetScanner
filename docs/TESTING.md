# 测试

[返回首页](../README.md)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

CTest 为所有测试设置 `QT_QPA_PLATFORM=offscreen`，无需显示器即可运行。测试只连接回环地址，不访问外部网络。

| 测试 | 内容 |
| --- | --- |
| `targets` | IPv4 / CIDR / 范围解析、规范十进制校验、去重、主机数上限、端口边界 |
| `scanner` | 回环地址上的真实开放与关闭端口、并发队列、停止与重启、回调重入 |
| `ui` | 回环扫描、CSV 导出与转义、收藏、搜索与隐藏、数值排序、控件状态、双击服务 URL、窗口生命周期 |
| `nmap` | 与 `nmap -sT -Pn -n` 逐端口对照 |

## Nmap 对照

`nmap` 测试在回环地址上开启 3 个监听端口和 3 个关闭端口，分别用 NetScanner 和 `nmap -sT -Pn -n --reason -vv` 扫描，要求每个端口的状态一致。查看详细输出：

```bash
ctest --test-dir build -V -R '^nmap$'
```

系统未安装 Nmap 时该测试会跳过，输出中显示 `SKIP`。

## 界面截图

设置 `NETSCANNER_SCREENSHOT` 后运行 UI 测试，会保存主窗口截图及展开高级参数后的截图（文件名加 `-advanced` 后缀）：

```bash
NETSCANNER_SCREENSHOT=$PWD/docs/ui-preview.png QT_QPA_PLATFORM=offscreen ./build/test_ui scanAndExport
```
