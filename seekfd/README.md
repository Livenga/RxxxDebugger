# seekfd

`peekfd` を参考に read/write するデータの監視.  
特定の環境下のみを想定して開発しているため, 現在汎用性はない.


### NOTEs
`ptrace(PTRACE_GETREGS)` で取得される汎用レジスタの第一引数に該当するレジスタに `/proc/[pid]/fd` に含まれていないような格納されている場合がある.  
```C
// uregs[4] ここが rdi の様な振る舞いをしていると思われる.
struct user_regs {
  unsigned long int uregs[18];
};
```
