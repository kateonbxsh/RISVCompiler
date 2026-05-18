```
NOP     _     _     _     ; no operation
AFC     Ri    imm   _     ; Ri = imm
COP     Ri    Rj    _     ; Ri = Rj

ADD     Ri    Rj    Rk    ; Ri = Rj + Rk
MUL     Ri    Rj    Rk    ; Ri = Rj * Rk
SUB     Ri    Rj    Rk    ; Ri = Rj - Rk
DIV     Ri    Rj    Rk    ; Ri = Rj / Rk

NOT     Ri    Rj    _     ; Ri = !Rj
AND     Ri    Rj    Rk    ; Ri = Rj && Rk
OR      Ri    Rj    Rk    ; Ri = Rj || Rk
BNOT    Ri    Rj    _     ; Ri = ~Rj
BAND    Ri    Rj    Rk    ; Ri = Rj & Rk
BOR     Ri    Rj    Rk    ; Ri = Rj | Rk

LOAD    Ri    @j    _     ; Ri = MEM[@j]
STR     @i    Rj    _     ; MEM[@i] = Rj
LDI     Ri    Rj    _     ; Ri = MEM[Rj]
STI     _     Ri    Rj    ; MEM[Ri] = Rj

J       _     addr  _     ; PC = addr
JF      _     Ri    addr  ; if Ri == 0, PC = addr
JI      _     Ri    _     ; PC = Ri

LT      Ri    Rj    Rk    ; Ri = Rj < Rk
GT      Ri    Rj    Rk    ; Ri = Rj > Rk
EQ      Ri    Rj    Rk    ; Ri = Rj == Rk
LEQ     Ri    Rj    Rk    ; Ri = Rj <= Rk
GEQ     Ri    Rj    Rk    ; Ri = Rj >= Rk
NEQ     Ri    Rj    Rk    ; Ri = Rj != Rk

PRI     _     Ri   _     ; print Ri
```
