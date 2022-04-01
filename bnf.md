# 仅为测试参考使用的BNF范式
num := [0-9.]
id := [a-z_]{a-zA-Z0-9_}
primary := "(" expr ")" | num | id | string
factor := "-" primary | primary
expr := factor { OP factor }
simple := expr
block := "{" [statement] {;|EOL| [statement] } "}"
statement := if expr block ["else" block] | "while" expr block | simple
program := [statement] (; | EOF)