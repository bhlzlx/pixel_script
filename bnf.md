# 仅为测试参考使用的BNF范式
num := [0-9.]
id := [a-z_]{a-zA-Z0-9_}
primary := "(" expr ")" | num | id | string
factor := func | "-" primary | primary
expr := factor { OP factor }
simple := expr
block := "{" [statement] {;|EOL| [statement] } "}"
statement := if expr block ["else" block] | "while" expr block | simple
program := {func|statement} (; | EOF)

args := id {, id}
args_decl := "(" [args] ")"
func := "func" id args_decl block
closure := "func" args_decl block