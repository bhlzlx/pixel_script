# 仅为测试参考使用的BNF范式
* num := [0-9.]
* id := [a-z_]{a-zA-Z0-9_}
* args := expr {, expr}
* postfix := "("[args]")"
* primary := "(" expr ")" | num | string | id [postfix]
* factor := closure | "-" primary | primary
* expr := factor { OP factor }
* def_var := "var" id [= expr]
* simple := expr
* block := "{" [statement] {;|EOL| [statement] } "}"
* statement := if expr block ["else" block] | "while" expr block | def_var | simple
* program := {func|statement} (; | EOF)
* args := id {, id}
* args_decl := "(" [args] ")"
* func := "func" id args_decl block
* closure := "func" args_decl block


* package := "package" {.id} EOL
* code_chunk := package { def_var | function } EOF

## 注释

### statement

语句块，流程控制类的代码，比如：
```cpp
if(expr) {
    ....
} else {
    ....
}
```

```cpp
while(expr) {
    ...
}
```
```cpp
a = 4
```
这类的语句可能是在其它的语句块里的，也可能是在全局范围内写的代码。

### expr

即字面意思，表达式，表达式有多种形式，可以是一个变量或者常量，也可以是一个计算公式（如 a+b*c/d），也可能是一个函数调用的返回值，也可能是前者各种类型组合的算式。

`a`  
`a = 8`  
`get_value(b)`  
`a + b * get_value()`  
