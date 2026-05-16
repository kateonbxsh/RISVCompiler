```
NOP     _     _     _     ; no operation
AFC     Ri    imm   _     ; Ri = imm
COP     Ri    Rj    _     ; Ri = Rj

ADD     Ri    Rj    Rk    ; Ri = Rj + Rk
MUL     Ri    Rj    Rk    ; Ri = Rj * Rk
SOU     Ri    Rj    Rk    ; Ri = Rj - Rk
DIV     Ri    Rj    Rk    ; Ri = Rj / Rk

NOT     Ri    Rj    _     ; Ri = !Rj
AND     Ri    Rj    Rk    ; Ri = Rj && Rk
OR      Ri    Rj    Rk    ; Ri = Rj || Rk
BNOT    Ri    Rj    _    ; Ri = ~Rj
BAND    Ri    Rj    Rk   ; Ri = Rj & Rk
BOR     Ri    Rj    Rk   ; Ri = Rj | Rk

LOAD    Ri    @j    _     ; Ri = MEM[@j]
STORE   @i    Rj    _     ; MEM[@i] = Rj
LOADR   Ri    Rj    _     ; Ri = MEM[Rj]
STORER  Ri    Rj    _     ; MEM[Ri] = Rj

JMP     addr  _     _     ; PC = addr
JMF     Ri    addr  _     ; if Ri == 0, PC = addr
JMPR    Ri    _     _     ; PC = Ri

LT     Ri    Rj    Rk    ; Ri = Rj < Rk
GT     Ri    Rj    Rk    ; Ri = Rj > Rk
EQ     Ri    Rj    Rk    ; Ri = Rj == Rk
LEQ     Ri    Rj    Rk    ; Ri = Rj <= Rk
GEQ     Ri    Rj    Rk    ; Ri = Rj >= Rk
NEQ     Ri    Rj    Rk    ; Ri = Rj != Rk

PRI     Ri    _     _     ; print Ri
```
