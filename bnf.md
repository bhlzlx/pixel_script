# 仅为测试参考使用的BNF范式
num := [0-9.]
id := [a-z_]{a-zA-Z0-9_}
primary := "(" expr ")" | num | id | string
factor := closure | "-" primary | primary
expr := factor { OP factor }
simple := expr
block := "{" [statement] {;|EOL| [statement] } "}"
statement := if expr block ["else" block] | "while" expr block | simple
program := {func|statement} (; | EOF)
args := id {, id}
args_decl := "(" [args] ")"
func := "func" id args_decl block
closure := "func" args_decl block


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
