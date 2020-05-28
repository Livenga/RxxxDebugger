# seekfd

`peekfd` を参考に read/write するデータの監視.  
特定の環境下(ARM 32bit \_\_ARM\_EABI\_\_ = 1)のみを想定して開発しているため, 現在汎用性はない.


### NOTEs
~~`ptrace(PTRACE_GETREGS)` で取得される汎用レジスタの第一引数に該当するレジスタに `/proc/[pid]/fd` に含まれていないような格納されている場合がある.~~


汎用レジスタ `r12(ip)` の状態により, 第一引数を格納するレジスタが変わる(下記ページ参照).  
[アーキテクチャ固有のレジスタ](http://www.katsuster.net/wiki/index.php?katsuhiro%2Frefmon%2Farm_port)  
[Assembly Programming on ARM Linux(02)](https://www.mztn.org/slasm/arm02.html)

```C
// r0 第一引数の調整
unsigned long int r0 = (*(regs.uregs + 12) == 0)
    ? *(regs.uregs + 0)
    : *(regs.uregs + 17)

// 返り値の調整
unsigned long int ret = (*(regs.uregs + 12) == 1)
    ? *(regs.uregs + 0)
    : 0;
```

```C
// Note: uregs 0-15 = r0..r15
//       r16 = CPSR
//       r17 = orig_r0
struct user_regs {
  unsigned long int uregs[18];
};
```


### TODO

#### 低い重要度
汎用的なソフトでなく, これからも使い続けていく可能性が低いため修正する可能性は低い.  
* pthread の利用をやめ, sigaction でプログラムの終了, プロセスの切り離しを行えるようにする.
