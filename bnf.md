# 仅为测试参考使用的BNF范式
* num := [0-9.]
* id := [a-z_]{a-zA-Z0-9_}
* args := expr {, expr}
* arg_list : "("[args]")"
* indexer : "[" expr "]"
* primary := "(" expr ")" | closure | num | string | id
* factor := map | array | "-" primary | primary { indexer | arg_list | ("." id) }
* array := "[" expr {, expr} "]"
* map_item := id:expr
* map := "{" map_item {, map_item} "}"
* expr := factor { OP factor }
* def_var := "var" id [= expr]
* simple := expr
* block := "{" [statement] {;|EOL| [statement] } "}"
* statement := "return" expr | if expr block ["else" block] | "while" expr block | def_var | simple
* program := {def_func|statement} (; | EOF)
* params := id {, id}
* param_list := "(" [params] ")"
* def_func := "func" id args_decl block
* closure := "func" args_decl block

* member : def_var | def_func
* class_body := "{" member {(EOL|;) member }  "}"
* def_class := "class" id ["extend" id] class_body

* package := "package" id {.id} EOL
* code_chunk := package { def_var | function } EOF


## 补充

* primary

```
(1+val)
(param, a, b, c) { return 0 }
123
"hello,world!"
id
```

### factor

```
包含 primary
-value
value.attr
value(a, b, get(c))
value[loc]
```

factor 代表一个值或者一个可以处理完作为一个值来运算，所以

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
