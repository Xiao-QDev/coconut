# Pico 编程语言

Pico 是用 C 实现的通用编程语言，伪代码风格语法，中英文关键字双写，即时反馈。

[![Build](https://github.com/ZerexaNet/Pico/actions/workflows/release.yml/badge.svg)](https://github.com/ZerexaNet/Pico/actions)

## 特性

- **双语语法**：`fn` / `函数`，`let` / `定义`，`if` / `如果` 等，中英文等价
- **标点兼容**：`，` `：` `（）` `【】` 自动等价于 ASCII 标点
- **三种块风格**：缩进块 / 大括号 `{}` / `开始`…`结束`
- **面向对象**：`struct` / `结构体`，方法，继承 `struct 子类(父类):`，`super`
- **并发**：`spawn` / `启动` 多线程，`Mutex` 互斥锁，`Channel` 通道，`yield` 生成器
> [!WARNING]
> 多线程、协程、网络功能在 WASM 环境下不可用（stub 实现，调用无效果）。
- **字节码 VM**：AST 编译为字节码，VM 执行（30+ 指令）
- **WASM 支持**：`pico_wasm_run()` 导出，emcc 编译为 `.wasm` 在浏览器运行
> [!WARNING]
> WASM 构建仅支持核心语言特性（运算、控制流、结构体、闭包）。线程、协程、网络、Qt GUI 均为 stub，调用无效果。
- **Qt GUI**：`ui.window` / `ui.button` / `ui.label` / `ui.input`（需 Qt 环境）
> [!WARNING]
> Qt 绑定需编译时加 `-DQT_AVAILABLE` 并链接 Qt 库。未启用时所有 `ui.*` 调用返回 nil。
- **类型系统**：可选静态标注 `let x: int = 5`，`fn f(x: int) -> str:`，`--strict` 模式启用运行时检查
- **包管理器**：`pico install owner/repo[@tag]`，`pico remove pkg`，`pico list`，包存 `pico_modules/`
- **标准库**：IO、字符串、列表、字典、数学、文件、HTTP 服务器、JSON
- **友好错误**：精确行列定位 + "你是否想说？"建议

## 快速开始

**文档：** https://pico.zerexa.cn

**下载：** [GitHub Releases](https://github.com/ZerexaNet/Pico/releases/latest)

```bash
# 从源码编译
git clone https://github.com/ZerexaNet/Pico.git
cd Pico && make
./pico          # 启动 REPL
./pico run hello.pico
```

## 示例

```
# Hello World
令 名字 = "世界"
打印(f"你好，{名字}！")

# 结构体与继承
结构体 动物:
    名字: str
    fn 说话(self): return f"{self.名字}..."

结构体 狗(动物):
    fn 说话(self): return f"{self.名字}：汪汪！"

令 d = 狗{名字: "旺财"}
打印(d.说话())

# 多线程
令 任务 = 启动 fn(): return 1 + 1
打印(任务.等待())

# Web 服务器
net.listen(8080, fn(路径):
    return f"<h1>访问了 {路径}</h1>"
)
```

## 目录结构

```
src/
  lexer.c / parser.c     词法与语法分析
  interpreter.c          树遍历求值引擎
  vm.c / compiler.c      字节码 VM 与编译器
  wasm_entry.c           WASM 导出入口
  value.c                动态类型系统
  thread.c / coroutine.c 并发与协程
  codegen.c              C 代码生成器
  stdlib/                标准库（net, json, qt_bind）
examples/                示例代码
tests/                   测试用例
```

## SDK

### 嵌入 SDK — 在 C 项目中运行 Pico

```c
#include "sdk/pico.h"

PicoVM *vm = pico_new();
pico_set_int(vm, "version", 1);
pico_register(vm, "c_add", my_c_fn);
pico_run_string(vm, "print(c_add(1, 2))");
pico_free(vm);
```

### 扩展 SDK — 用 C 写 Pico 原生模块

```c
#include "sdk/pico_ext.h"

PICO_MODULE(mymod) {
    PICO_EXPORT("sqrt", my_sqrt);
    PICO_EXPORT("pow",  my_pow);
}
```

```bash
# 编译嵌入示例
gcc -std=c11 -Isdk -Isrc examples/sdk/embed_example.c sdk/pico.c src/*.c src/stdlib/*.c -lm -lpthread -o embed_example
```

完整示例见 [examples/sdk/](examples/sdk/)，API 文档见 [sdk/pico.h](sdk/pico.h) 和 [sdk/pico_ext.h](sdk/pico_ext.h)。

## 路线图

- [x] MVP 解释器（词法、语法、求值）
- [x] 结构体、字典、闭包
- [x] 协程 / 生成器（yield）
- [x] 多线程（Mutex、Channel）
- [x] 面向对象继承（super）
- [x] 标准库（IO、Net、JSON）
- [x] 字节码虚拟机（VM）
- [x] WASM 兼容（Web 部署）
- [x] Qt 图形界面绑定
- [x] 类型系统（可选静态标注）
- [x] 包管理器

## 许可证

Apache 2.0
