from tokenizer import T, Token, Tokenizer
from enum import Enum, auto

class I(Enum):
    Label = 0
    LocalVariable = auto()

class Identifier:
    def __init__(self, type, value, size=8):
        self.type = type
        self.value = value
        self.size = size

class Compiler:
    def __init__(self, instream, outstream):
        self.tokenizer = Tokenizer(instream)
        self.outstream = outstream

    def run(self):
        self.idents = {}
        self.cur_label = ""
        self.bruh = {}
        self.strs = {}
        self.str_off = 0
        self.is_kc_expr = False
        self.first_pass = True
        self.compile()
        self.tokenizer.reset()
        self.first_pass = False
        self.data_begin = self.ip
        self.compile()

        for s in self.strs:
            self.emit16(len(s))
            self.emit16(self.ip + 2)
            for c in s:
                self.emit8(ord(c))

    def compile(self):
        self.prev = Token(None, None)
        self.cur = Token(None, None)
        self.advance()

        self.ip = self.origin = 0

        while not self.end(): self.statement()

    def statement(self):
        t = self.advance()
        match t.type:
            case T.SemiColon: pass
            case T.Org: self.ip = self.origin = self.expression()
            case T.Add: self.grp1(0x00, 0)
            case T.Or: self.grp1(0x08, 1)
            case T.Adc: self.grp1(0x10, 2)
            case T.Sbb: self.grp1(0x18, 3)
            case T.And: self.grp1(0x20, 4)
            case T.Sub: self.grp1(0x28, 5)
            case T.Xor: self.grp1(0x30, 6)
            case T.Cmp: self.grp1(0x38, 7)
            case T.Daa: self.emit8(0x27)
            case T.Aaa: self.emit8(0x37)
            case T.Nop: self.emit8(0x90)
            case T.Movsb: self.emit8(0xA4)
            case T.Movsw: self.emit8(0xA5)
            case T.Cmpsb: self.emit8(0xA6)
            case T.Cmpsw: self.emit8(0xA7)
            case T.Ret: self.emit8(0xC3)
            case T.Xlatb: self.emit8(0xD7)
            case T.Lock: self.emit8(0xF0)
            case T.Repnz: self.emit8(0xF2)
            case T.Repz: self.emit8(0xF3)
            case T.Hlt: self.emit8(0xF4)
            case T.Cmc: self.emit8(0xF5)
            case T.Das: self.emit8(0x2F)
            case T.Aas: self.emit8(0x3F)
            case T.Cbw: self.emit8(0x98)
            case T.Cwd: self.emit8(0x99)
            case T.Wait: self.emit8(0x9B)
            case T.Pushf: self.emit8(0x9C)
            case T.Popf: self.emit8(0x9D)
            case T.Sahf: self.emit8(0x9E)
            case T.Lahf: self.emit8(0x9F)
            case T.Stosb: self.emit8(0xAA)
            case T.Stosw: self.emit8(0xAB)
            case T.Lodsb: self.emit8(0xAC)
            case T.Lodsw: self.emit8(0xAD)
            case T.Scasb: self.emit8(0xAE)
            case T.Scasw: self.emit8(0xAF)
            case T.Retf: self.emit8(0xCB)
            case T.Into: self.emit8(0xCE)
            case T.Iret: self.emit8(0xCF)
            case T.Clc: self.emit8(0xF8)
            case T.Stc: self.emit8(0xF9)
            case T.Cli: self.emit8(0xFA)
            case T.Sti: self.emit8(0xFB)
            case T.Cld: self.emit8(0xFC)
            case T.Std: self.emit8(0xFD)
            case T.Rol: self.grp2(0)
            case T.Ror: self.grp2(1)
            case T.Rcl: self.grp2(2)
            case T.Rcr: self.grp2(3)
            case T.Shl: self.grp2(4)
            case T.Shr: self.grp2(5)
            case T.Sar: self.grp2(7)
            case T.Not: self.grp3(2)
            case T.Neg: self.grp3(3)
            case T.Mul: self.grp3(4)
            case T.Imul: self.grp3(5)
            case T.Div: self.grp3(6)
            case T.Idiv: self.grp3(7)
            case T.Jmp: self.jmp(0xE9, 0xEA, 4)
            case T.Call: self.jmp(0xE8, 0x9A, 2)
            case T.Jo: self.jmp8(0x70)
            case T.Jno: self.jmp8(0x71)
            case T.Jb: self.jmp8(0x72)
            case T.Jnb: self.jmp8(0x73)
            case T.Jz: self.jmp8(0x74)
            case T.Jnz: self.jmp8(0x75)
            case T.Jbe: self.jmp8(0x76)
            case T.Ja: self.jmp8(0x77)
            case T.Js: self.jmp8(0x78)
            case T.Jns: self.jmp8(0x79)
            case T.Jpe: self.jmp8(0x7A)
            case T.Jpo: self.jmp8(0x7B)
            case T.Jl: self.jmp8(0x7C)
            case T.Jge: self.jmp8(0x7D)
            case T.Jle: self.jmp8(0x7E)
            case T.Jg: self.jmp8(0x7F)
            case T.Loopnz: self.jmp8(0xE0)
            case T.Loopz: self.jmp8(0xE1)
            case T.Loop: self.jmp8(0xE2)
            case T.Jcxz: self.jmp8(0xE3)
            case T.Mov: self.mov()
            case T.Identifier: self.ident(t.value)
            case T.Dot: self.label(self.cur_label + self.consume(T.Identifier).value, False)
            case T.Db: self.dx(self.emit8, self.imm8)
            case T.Dw: self.dx(self.emit16, self.imm16)
            case T.Dd: self.dx(self.emit32, self.imm32)
            case T.Rb: self.ip += self.expression()
            case T.Pad: self.pad()
            case T.Int: self.int()
            case T.Inc: self.incdec(0x40, 0)
            case T.Dec: self.incdec(0x48, 1)
            case T.Test: self.test()
            case T.Lea: self.lea(0x8D)
            case T.Les: self.lea(0xC4)
            case T.Lds: self.lea(0xC5)
            case T.Push: self.pushpop(True)
            case T.Pop: self.pushpop(False)
            case T.Xchg: self.xchg()
            case T.Function: self.function()
            case T.While: self.while_()
            case T.Const: self.const()
            case T.Struct: self.struct()
            case T.If: self.if_()
            case T.Return: self.ret()
            case T.Star: self.deref_assign()
            case x: self.die(f"unexpected token {x}")

    def deref_assign(self):
        deref = 1
        while self.match(T.Star).type: deref += 1
        ident = self.consume(T.Identifier).value
        f = self.assign(ident, True)
        for i in range(deref):
            if f and i == 0:
                self.emit8(0x8B)
                self.emit8(self.modrm(2, 3, 0b110))
                x = 0 if ident not in self.idents else self.idents[ident].value
                self.emit16(x) # MOV BX, [var]
            else:
                self.emit8(0x8B)
                self.emit8(self.modrm(0, 3, 0b111)) # MOV BX, [BX]
        self.emit8(0x53) # PUSH BX
        self.kc_expression()
        self.emit8(0x5B) # POP BX
        self.emit8(0x89)
        self.emit8(self.modrm(0, 0, 0b111)) # MOV [BX], AX
        self.consume(T.SemiColon)

    def ret(self):
        self.kc_expression()
        self.consume(T.SemiColon)
        self.emit8(0x8B)
        self.emit8(self.modrm(0b11, 4, 5)) # MOV SP, BP
        self.emit8(0x5D) # POP BP
        self.emit8(0xC3) # RET

    def const(self):
        ident = self.consume(T.Identifier).value
        self.consume(T.Equals)
        val = self.expression()
        if ident in self.idents and self.first_pass: self.die("identifier redefined")
        self.idents[ident] = Identifier(I.Label, val)
        self.consume(T.SemiColon)

    def struct(self):
        struct = self.consume(T.Identifier).value
        self.consume(T.Equals)
        i = 0
        while True:
            ident = self.consume(T.Identifier).value
            if ident in self.idents and self.first_pass: self.die("identifier redefined")
            self.idents[ident] = Identifier(I.Label, i)
            i += 1
            if self.match(T.Comma).type: continue
            break
        if struct in self.idents and self.first_pass: self.die("identifier redefined")
        self.idents[struct] = Identifier(I.Label, i)
        self.consume(T.SemiColon);

    def function(self):
        ident = self.consume(T.Identifier).value
        self.idents[ident] = Identifier(I.Label, self.ip)
        self.cur_label = ident
        self.consume(T.LeftParen)
        pop = []
        while not self.match(T.RightParen).type:
            arg = self.consume(T.Identifier).value
            pop.append(arg)
            self.idents[arg] = Identifier(I.LocalVariable, 0)
            if self.match(T.Comma).type: continue
            self.consume(T.RightParen)
            break
        for i, v in reversed(list(enumerate(pop))):
            self.idents[v].value = (i + 2) * 2
        la = 0
        x = -2
        self.emit8(0x55) # PUSH BP
        self.emit8(0x8B)
        self.emit8(self.modrm(0b11, 5, 4)) # MOV BP, SP
        if self.match(T.LeftParen).type:
            while not self.match(T.RightParen).type:
                arg = self.consume(T.Identifier).value
                pop.append(arg)
                size = 2
                n = False
                if self.match(T.LeftBracket).type:
                    size = self.expression() * 2 + 2
                    self.consume(T.RightBracket)
                    n = True
                elif self.match(T.LeftBrace).type:
                    size = self.expression() + 2
                    self.consume(T.RightBrace)
                    n = True
                if n:
                    self.emit8(0x8D)
                    self.emit8(self.modrm(2, 0, 0b110))
                    self.emit16(x - size + 2) # LEA AX, [BP-n]
                    self.emit8(0x89)
                    self.emit8(self.modrm(2, 0, 0b110))
                    self.emit16(x) # MOV [BP-n], AX
                self.idents[arg] = Identifier(I.LocalVariable, x)
                x -= size
                la += size
                if self.match(T.Comma).type: continue
                self.consume(T.RightParen)
                break
        self.emit8(0x81)
        self.emit8(self.modrm(0b11, 5, 4))
        self.emit16(la) # SUB SP, la
        self.statements()
        for x in pop: del self.idents[x]
        self.emit8(0x8B)
        self.emit8(self.modrm(0b11, 4, 5)) # MOV SP, BP
        self.emit8(0x5D) # POP BP
        self.emit8(0xC3) # RET

    def while_(self):
        self.consume(T.LeftParen)
        lbl1 = self.ip
        self.kc_expression()
        self.consume(T.RightParen)
        self.emit8(0x85)
        self.emit8(self.modrm(0b11, 0, 0)) # TEST AX, AX
        self.emit8(0x75)
        self.emit8(3) # JNZ +3
        a = self.ip
        self.emit8(0xE9)
        br = self.ip
        if self.first_pass: lbl3 = 0
        else: lbl3 = self.bruh[self.ip]
        self.emit16(lbl3 - a - 3) # JMP lbl2
        self.statements()
        lbl2 = self.ip
        if lbl2 - lbl1 > 124:
            self.emit8(0xE9)
            self.emit16(lbl1 - lbl2 - 3) # JMP lbl1
        else:
            self.emit8(0xEB)
            self.emit8(lbl1 - lbl2 - 2) # JMP SHORT lbl1
        if self.first_pass: self.bruh[br] = self.ip

    def if_(self):
        self.consume(T.LeftParen)
        self.kc_expression()
        br = self.ip
        if self.first_pass: lbl = 0
        else: lbl = self.bruh[self.ip]
        self.emit8(0x85)
        self.emit8(self.modrm(0b11, 0, 0)) # TEST AX, AX
        self.emit8(0x75)
        self.emit8(3) # JNZ +3
        a = self.ip
        self.emit8(0xE9)
        self.emit16(lbl - a - 3) # JMP lbl
        self.consume(T.RightParen)
        self.statements()
        if self.match(T.Else).type:
            br3 = self.ip
            if self.first_pass: lbl3 = 0
            else: lbl3 = self.bruh[self.ip]
            self.emit8(0xE9)
            self.emit16(lbl3 - br3 - 3) # JMP lbl3
            if self.first_pass: self.bruh[br] = self.ip
            self.statements()
            if self.first_pass: self.bruh[br3] = self.ip
        else:
            if self.first_pass: self.bruh[br] = self.ip

    def statements(self):
        if self.match(T.LeftBrace).type:
            while not self.match(T.RightBrace).type:
                self.statement()
        else:
            self.statement()

    def ident(self, ident):
        if self.peek().type == T.Colon: return self.label(ident)
        if self.peek().type == T.LeftParen: return self.call(ident)
        return self.assign(ident)

    def call(self, ident, s=False):
        self.consume(T.LeftParen)
        br = self.ip
        if self.first_pass: c = 0
        else: c = self.bruh[self.ip]
        self.emit8(0x81)
        self.emit8(self.modrm(0b11, 5, 4))
        self.emit16(c * 2) # SUB SP, c * 2
        i = 0
        e = c
        while not self.match(T.RightParen).type:
            self.kc_expression()
            self.emit8(0x8B)
            self.emit8(self.modrm(0b11, 3, 4)) # MOV BX, SP
            self.emit8(0x89)
            self.emit8(self.modrm(2, 0, 0b111))
            self.emit16(i * 2) # MOV [BX+i*2], AX
            i += 1
            e -= 1
            if self.match(T.Comma).type: continue
            break
        if self.first_pass: self.bruh[br] = i
        if self.first_pass: x = 0
        else:
            if ident in self.idents and self.idents[ident].type == I.Label:
                x = self.idents[ident].value
        if ident in self.idents and self.idents[ident].type == I.LocalVariable:
            x = self.idents[ident].value
            self.emit8(0xFF)
            self.emit8(self.modrm(2, 2, 0b110))
            self.emit16(x) # CALL [var]
        else:
            a = self.ip
            self.emit8(0xE8)
            self.emit16(x - a - 3) # CALL ident
        self.emit8(0x81)
        self.emit8(self.modrm(0b11, 0, 4))
        self.emit16(c * 2) # ADD SP, c * 2
        self.consume(T.RightParen)
        if not s: self.consume(T.SemiColon)

    def label(self, ident, glbl=True):
        self.consume(T.Colon)
        if glbl: self.cur_label = ident
        if not self.first_pass: return
        if ident in self.idents: self.die(f"redefined label {ident}")
        self.idents[ident] = Identifier(I.Label, self.ip)

    def assign(self, ident, s=False):
        f = True
        b = False
        while not self.match(T.Equals).type:
            if self.match(T.LeftBracket).type:
                if f:
                    self.emit8(0x8B)
                    self.emit8(self.modrm(2, 3, 0b110))
                    x = 0 if ident not in self.idents else self.idents[ident].value
                    self.emit16(x) # MOV BX, [var]
                else:
                    self.emit8(0x8B)
                    self.emit8(self.modrm(0, 3, 0b111)) # MOV BX, [BX]
                self.emit8(0x53) # PUSH BX
                self.kc_expression()
                self.consume(T.RightBracket)
                self.emit8(0x5B) # POP BX
                self.emit8(0x96) # XCHG SI, AX
                self.emit8(0xD1)
                self.emit8(self.modrm(0b11, 4, 6)) # SHL SI, 1
                self.emit8(0x8D)
                self.emit8(self.modrm(0, 3, 0b000)) # LEA BX, [BX+SI]
                f = False
                b = False
            elif self.match(T.LeftBrace).type:
                if f:
                    self.emit8(0x8B)
                    self.emit8(self.modrm(2, 3, 0b110))
                    x = 0 if ident not in self.idents else self.idents[ident].value
                    self.emit16(x) # MOV BX, [var]
                else:
                    self.emit8(0x8B)
                    self.emit8(self.modrm(0, 3, 0b111)) # MOV BX, [BX]
                self.emit8(0x53) # PUSH BX
                self.kc_expression()
                self.consume(T.RightBrace)
                self.emit8(0x5B) # POP BX
                self.emit8(0x96) # XCHG SI, AX
                self.emit8(0x8D)
                self.emit8(self.modrm(0, 3, 0b000)) # LEA BX, [BX+SI]
                f = False
                b = True
            else:
                self.die("unexpected token")
        if s: return f
        if not f:
            self.emit8(0x53) # PUSH BX
        self.kc_expression()
        if not f:
            self.emit8(0x5B) # POP BX
        self.consume(T.SemiColon)
        if f:
            self.emit8(0x89)
            self.emit8(self.modrm(2, 0, 0b110))
            self.emit16(self.idents[ident].value) # MOV [var], AX
        else:
            if b:
                self.emit8(0x88)
                self.emit8(self.modrm(0, 0, 0b111)) # MOV [BX], AL
            else:
                self.emit8(0x89)
                self.emit8(self.modrm(0, 0, 0b111)) # MOV [BX], AX

    def dx(self, emit, imm):
        while True:
            if self.peek().type == T.Chars:
                for c in self.advance().value: emit(ord(c))
            else:
                emit(imm())

            if self.match(T.Comma).type: continue
            break

    def pad(self):
        times = self.expression()
        self.consume(T.Comma)
        b = self.imm8()
        for _ in range(times): self.emit8(b)

    def int(self):
        v = self.imm8()
        self.emit8(0xCD)
        self.emit8(v)

    def incdec(self, base, reg):
        if (d := self.reg16()) != None:
            self.emit8(base + d)
        elif (d := self.reg8()) != None:
            self.emit8(0xFE)
            self.emit8(self.modrm(0b11, reg, d))
        elif (ssmrmd := self.mem()) != None:
            size, seg, mod, rm, disp = ssmrmd
            if size == None: self.die("unknown operand size")
            if seg != None: self.emit8(seg)
            self.emit8(0xFE if size == 8 else 0xFF)
            self.emit8(self.modrm(mod, reg, rm))
            if disp != None: self.emit16(disp)

    def test(self):
        if (d := self.reg8()) != None:
            self.consume(T.Comma)
            if (s := self.reg8()) != None:
                self.emit8(0x84)
                self.emit8(self.modrm(0b11, d, s))
            elif (ssmrmd := self.mem()) != None:
                size, seg, mod, rm, disp = ssmrmd
                if size != 8 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(0x84)
                self.emit8(self.modrm(mod, d, rm))
                if disp != None: self.emit16(disp)
            else:
                v = self.imm8()
                self.emit8(0xF6)
                self.emit8(self.modrm(0b11, 0, d))
                self.emit8(v)
        elif (d := self.reg16()) != None:
            self.consume(T.Comma)
            if (s := self.reg16()) != None:
                self.emit8(0x85)
                self.emit8(self.modrm(0b11, d, s))
            elif (ssmrmd := self.mem()) != None:
                size, seg, mod, rm, disp = ssmrmd
                if size != 16 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(0x85)
                self.emit8(self.modrm(mod, d, rm))
                if disp != None: self.emit16(disp)
            else:
                v = self.imm16()
                self.emit8(0xF7)
                self.emit8(self.modrm(0b11, 0, d))
                self.emit16(v)
        else:
            self.die("unsupported instruction")

    def lea(self, op):
        if (d := self.reg16()) == None: self.die("unsupported instruction")
        self.consume(T.Comma)
        if (ssmrmd := self.mem()) == None: self.die("unsupported instruction")
        size, seg, mod, rm, disp = ssmrmd
        if size != None and size != 16: self.die("operand size mismatch")
        if seg != None: self.emit8(seg)
        self.emit8(op)
        self.emit8(self.modrm(mod, d, rm))
        if disp != None: self.emit16(disp)

    def pushpop(self, push):
        if (d := self.reg16()) != None:
            if push: self.emit8(0x50 + d)
            else: self.emit8(0x58 + d)
        elif (t := self.match(T.Es, T.Cs, T.Ss, T.Ds).type) != None:
            if push: off = 0
            else: off = 1
            match t:
                case T.Es: off += 0x06
                case T.Cs: off += 0x0E
                case T.Ss: off += 0x16
                case T.Ds: off += 0x1E
            self.emit8(off)
        elif (ssmrmd := self.mem()) != None and push:
            size, seg, mod, rm, disp = ssmrmd
            if size != 16 and size != 0: self.die("operand size mismatch")
            if seg != None: self.emit8(seg)
            self.emit8(0xFF)
            self.emit8(self.modrm(mod, 6, rm))
            if disp != None: self.emit16(disp)
        else:
            self.die("unsupported instruction")

    def xchg(self):
        if (d := self.reg16()) != None:
            self.consume(T.Comma)
            if (s := self.reg16()) != None:
                self.emit8(0x87)
                self.emit8(self.modrm(0b11, d, s))
            else:
                self.die("unsupported instruction")
        else:
            self.die("unsupported instruction")

    def grp1(self, base, reg):
        if (d := self.reg8()) != None:
            self.consume(T.Comma)
            if (s := self.reg8()) != None:
                self.emit8(base + 2)
                self.emit8(self.modrm(0b11, d, s))
            elif (ssmrmd := self.mem()) != None:
                size, seg, mod, rm, disp = ssmrmd
                if size != 8 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(base + 2)
                self.emit8(self.modrm(mod, d, rm))
                if disp != None: self.emit16(disp)
            else:
                s = self.imm8()
                self.emit8(0x80)
                self.emit8(self.modrm(0b11, reg, d))
                self.emit8(s)
        elif (d := self.reg16()) != None:
            self.consume(T.Comma)
            if (s := self.reg16()) != None:
                self.emit8(base + 3)
                self.emit8(self.modrm(0b11, d, s))
            elif (ssmrmd := self.mem()) != None:
                size, seg, mod, rm, disp = ssmrmd
                if size != 16 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(base + 3)
                self.emit8(self.modrm(mod, d, rm))
                if disp != None: self.emit16(disp)
            else:
                s = self.imm16()
                self.emit8(0x81)
                self.emit8(self.modrm(0b11, reg, d))
                self.emit16(s)
        elif (ssmrmd := self.mem()) != None:
            size, seg, mod, rm, disp = ssmrmd
            self.consume(T.Comma)
            if (s := self.reg16()) != None:
                if size != 16 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(base + 1)
                self.emit8(self.modrm(mod, s, rm))
                if disp != None: self.emit16(disp)
            elif (s := self.reg8()) != None:
                if size != 8 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(base)
                self.emit8(self.modrm(mod, s, rm))
                if disp != None: self.emit16(disp)
            else:
                s = self.imm8() if size == 8 else self.imm16()
                if seg != None: self.emit8(seg)
                if size == 8: self.emit8(0x80)
                elif size == 16: self.emit8(0x81)
                else: self.die("operand size unknown")
                self.emit8(self.modrm(mod, reg, rm))
                if disp != None: self.emit16(disp)
                if size == 8: self.emit8(s)
                else: self.emit16(s)
        else:
            self.die("unsupported instruction")

    def grp2(self, reg):
        if (d := self.reg8()) != None:
            self.consume(T.Comma)
            if self.match(T.Cl).type:
                self.emit8(0xD2)
                self.emit8(self.modrm(0b11, reg, d))
            elif self.match(T.Number).value == 1:
                self.emit8(0xD0)
                self.emit8(self.modrm(0b11, reg, d))
            else:
                self.die("unsupported instruction")
        elif (d := self.reg16()) != None:
            self.consume(T.Comma)
            if self.match(T.Cl).type:
                self.emit8(0xD3)
            elif self.match(T.Number).value == 1:
                self.emit8(0xD1)
            else:
                self.die("unsupported instruction")
            self.emit8(self.modrm(0b11, reg, d))
        elif (smrmd := self.mem()) != None:
            size, seg, mod, rm, disp = smrmd
            if size == None: self.die("unknown operand size")
            self.consume(T.Comma)
            if seg != None: self.emit8(seg)
            if self.match(T.Cl).type:
                self.emit8(0xD3 if size == 16 else 0xD2)
            elif self.match(T.Number).value == 1:
                self.emit8(0xD1 if size == 16 else 0xD0)
            else:
                self.die("unsupported instruction")
            self.emit8(self.modrm(mod, reg, rm))
            if disp != None: self.emit16(disp)
        else:
            self.die("unsupported instruction")

    def grp3(self, reg):
        if (d := self.reg8()) != None:
            self.emit8(0xF6)
            self.emit8(self.modrm(0b11, reg, d))
        elif (d := self.reg16()) != None:
            self.emit8(0xF7)
            self.emit8(self.modrm(0b11, reg, d))
        elif (ssmrmd := self.mem()) != None:
            size, seg, mod, rm, disp = ssmrmd
            if size == None: self.die("unknown operand size")
            if seg != None: self.emit8(seg)
            if size == 8: self.emit8(0xF6)
            else: self.emit8(0xF7)
            self.emit8(self.modrm(mod, reg, rm))
            if disp != None: self.emit16(disp)
        else:
            self.die("unsupported instruction")

    def jmp(self, op, opfar, reg):
        if (d := self.reg16()) != None:
            self.emit8(0xFF)
            self.emit8(self.modrm(0b11, reg, d))
        elif ((f := self.match(T.Far).type) and (ssmrmd := self.mem()) != None) or (ssmrmd := self.mem()) != None:
            size, seg, mod, rm, disp = ssmrmd
            if size != 16 and size != None: self.die("unsupported operand size")
            if seg != None: self.emit8(seg)
            if f != None: reg += 1
            self.emit8(0xFF)
            self.emit8(self.modrm(mod, reg, rm))
            if disp != None: self.emit16(disp)
        else:
            a = self.imm16()
            if self.match(T.Colon).type:
                b = self.imm16()
                self.emit8(opfar)
                self.emit16(b)
                self.emit16(a)
            else:
                x = a - self.ip - 3
                self.emit8(op)
                self.emit16(x)

    def jmp8(self, op):
        v = self.imm16() - self.ip - 2
        if not self.first_pass and v < -127 or v > 255: self.die("immediate too large")
        self.emit8(op)
        self.emit8(v)

    def mov(self):
        if (d := self.reg8()) != None:
            self.consume(T.Comma)
            if (s := self.reg8()) != None:
                self.emit8(0x8A)
                self.emit8(self.modrm(0b11, d, s))
            elif (ssmrmd := self.mem()) != None:
                size, seg, mod, rm, disp = ssmrmd
                if size != 8 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(0x8A)
                self.emit8(self.modrm(mod, d, rm))
                if disp != None: self.emit16(disp)
            else:
                s = self.imm8()
                self.emit8(0xC6)
                self.emit8(self.modrm(0b11, 0, d))
                self.emit8(s)
        elif (d := self.reg16()) != None:
            self.consume(T.Comma)
            if (s := self.reg16()) != None:
                self.emit8(0x8B)
                self.emit8(self.modrm(0b11, d, s))
            elif (ssmrmd := self.mem()) != None:
                size, seg, mod, rm, disp = ssmrmd
                if size != 16 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(0x8B)
                self.emit8(self.modrm(mod, d, rm))
                if disp != None: self.emit16(disp)
            elif (s := self.seg()) != None:
                self.emit8(0x8C)
                self.emit8(self.modrm(0b11, s, d))
            else:
                s = self.imm16()
                self.emit8(0xC7)
                self.emit8(self.modrm(0b11, 0, d))
                self.emit16(s)
        elif (ssmrmd := self.mem()) != None:
            size, seg, mod, rm, disp = ssmrmd
            self.consume(T.Comma)
            if (s := self.reg16()) != None:
                if size != 16 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(0x89)
                self.emit8(self.modrm(mod, s, rm))
                if disp != None: self.emit16(disp)
            elif (s := self.reg8()) != None:
                if size != 8 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(0x88)
                self.emit8(self.modrm(mod, s, rm))
                if disp != None: self.emit16(disp)
            elif (s := self.seg()) != None:
                if seg != None: self.emit8(seg)
                self.emit8(0x8C)
                self.emit8(self.modrm(mod, s, rm))
                if disp != None: self.emit16(disp)
            else:
                s = self.imm8() if size == 8 else self.imm16()
                if seg != None: self.emit8(seg)
                if size == 8: self.emit8(0xC6)
                elif size == 16: self.emit8(0xC7)
                else: self.die("operand size unknown")
                self.emit8(self.modrm(mod, 0, rm))
                if disp != None: self.emit16(disp)
                if size == 8: self.emit8(s)
                else: self.emit16(s)
        elif (d := self.seg()) != None:
            self.consume(T.Comma)
            if (s := self.reg16()) != None:
                self.emit8(0x8E)
                self.emit8(self.modrm(0b11, d, s))
            elif (ssmrmd := self.mem()) != None:
                size, seg, mod, rm, disp = ssmrmd
                if size != 16 and size != None: self.die("operand size mismatch")
                if seg != None: self.emit8(seg)
                self.emit8(0x8E)
                self.emit8(self.modrm(mod, d, rm))
                if disp != None: self.emit16(disp)
            else:
                self.die("unsupported instruction")
        else:
            self.die("unsupported instruction")

    def reg8(self):
        match self.match(T.Al, T.Cl, T.Dl, T.Bl, T.Ah, T.Ch, T.Dh, T.Bh).type:
            case T.Al: return 0
            case T.Cl: return 1
            case T.Dl: return 2
            case T.Bl: return 3
            case T.Ah: return 4
            case T.Ch: return 5
            case T.Dh: return 6
            case T.Bh: return 7

    def reg16(self):
        match self.match(T.Ax, T.Cx, T.Dx, T.Bx, T.Sp, T.Bp, T.Si, T.Di).type:
            case T.Ax: return 0
            case T.Cx: return 1
            case T.Dx: return 2
            case T.Bx: return 3
            case T.Sp: return 4
            case T.Bp: return 5
            case T.Si: return 6
            case T.Di: return 7

    def seg(self):
        match self.match(T.Es, T.Cs, T.Ss, T.Ds).type:
            case T.Es: return 0
            case T.Cs: return 1
            case T.Ss: return 2
            case T.Ds: return 3

    def mem(self):
        size = disp = rm = None
        mod = 0
        if self.match(T.Byte).type:
            size = 8
        elif self.match(T.Word).type:
            size = 16

        if not self.match(T.LeftBracket).type: return None

        seg = self.match(T.Es, T.Cs, T.Ss, T.Ds).type
        if seg != None:
            self.consume(T.Colon)
            match seg:
                case T.Es: seg = 0x26
                case T.Cs: seg = 0x2E
                case T.Ss: seg = 0x36
                case T.Ds: seg = 0x3E

        base = self.match(T.Bp, T.Bx, T.Di, T.Si).type
        match base:
            case T.Bp:
                disp = 0
                rm = 0b110
            case T.Bx: rm = 0b111
            case T.Di: rm = 0b101
            case T.Si: rm = 0b100

        x = self.peek()
        if not base and x.type == T.Identifier and x.value in self.idents and self.idents[x.value].type == I.LocalVariable:
            disp = self.idents[self.advance().value].value
            rm = 0b110
            base = T.Bp

        index = None
        noindex = False
        if self.match(T.Plus).type:
            index = self.match(T.Si, T.Di).type
            if not index: noindex = True
            elif base == T.Bx:
                if index == T.Si: rm = 0b000
                elif index == T.Di: rm = 0b001
                else: self.die("invalid index register")
            elif base == T.Bp:
                disp = None
                if index == T.Si: rm = 0b010
                elif index == T.Di: rm = 0b011
                else: self.die("invalid index register")
            else:
                self.die("invalid base register")

        if (noindex or rm == None or self.match(T.Plus).type) and self.peek().type != T.RightBracket:
            if disp == None: disp = self.imm16()
            else: disp += self.imm16()
            if rm == None: rm = 0b110
            else: mod = 2

        if base == T.Bp and index == None: mod = 2
        self.consume(T.RightBracket)
        return (size, seg, mod, rm, disp)

    def modrm(self, mod, reg, rm):
        return (mod << 6) | (reg << 3) | rm

    def imm8(self):
        v = self.expression()
        if not self.first_pass and v < -128 or v > 255: self.die("immediate too large")
        return v

    def imm16(self):
        v = self.expression()
        if not self.first_pass and v < -32768 or v > 65535: self.die("immediate too large")
        return v

    def imm32(self):
        v = self.expression()
        if not self.first_pass and v < -2147483648 or v > 4294967295: self.die("immediate too large")
        return v

    def kc_expression(self):
        self.is_kc_expr = True
        self.expression()
        self.is_kc_expr = False

    def expression(self):
        return self.ternary()

    def ternary(self):
        x = self.logical_and()
        if not self.is_kc_expr: return x
        if self.match(T.Ternary).type:
            br = self.ip
            if self.first_pass: lbl = 0
            else: lbl = self.bruh[self.ip]
            self.emit8(0x85)
            self.emit8(self.modrm(0b11, 0, 0)) # TEST AX, AX
            self.emit8(0x75)
            self.emit8(3) # JNZ +3
            a = self.ip
            self.emit8(0xE9)
            self.emit16(lbl - a - 3) # JMP lbl
            self.logical_and()
            br3 = self.ip
            if self.first_pass: lbl3 = 0
            else: lbl3 = self.bruh[self.ip]
            self.emit8(0xE9)
            self.emit16(lbl3 - br3 - 3) # JMP lbl3
            if self.first_pass: self.bruh[br] = self.ip
            self.consume(T.Colon)
            self.logical_and()
            if self.first_pass: self.bruh[br3] = self.ip

    def logical_and(self):
        return self.binary(self.equality, T.LogicalAnd)

    def equality(self):
        return self.binary(self.comparison, T.Equals, T.NotEquals)

    def comparison(self):
        return self.binary(self.bitwise, T.GreaterThanU, T.GreaterThanS, T.LessThanU, T.LessThanS, T.GreaterEqualsU, T.GreaterEqualsS, T.LessEqualsU, T.LessEqualsS)

    def bitwise(self):
        return self.binary(self.term, T.BitwiseShiftRight, T.BitwiseShiftLeft, T.BitwiseXor, T.BitwiseXor, T.BitwiseOr, T.BitwiseAnd)

    def term(self):
        return self.binary(self.factor, T.Plus, T.Minus)

    def factor(self):
        return self.binary(self.unary, T.ModuloU, T.SlashU, T.StarU, T.ModuloS, T.SlashS, T.StarS)

    def c_binary(self, fn, *toks):
        r = fn()
        while t := self.match(*toks).type:
            match t:
                case T.BitwiseShiftRight: r >>= fn()
                case T.BitwiseShiftLeft: r <<= fn()
                case T.BitwiseXor: r ^= fn()
                case T.BitwiseOr: r |= fn()
                case T.BitwiseAnd: r &= fn()
                case T.Plus: r += fn()
                case T.Minus: r -= fn()
                case T.ModuloS | T.ModuloU: r %= fn()
                case T.SlashS | T.SlashU: r //= fn()
                case T.StarS | T.StarU: r *= fn()
                case T.GreaterThanS: r = 1 if r > fn() else 0
                case T.GreaterEqualsS: r = 1 if r >= fn() else 0
                case T.LessThanS: r = 1 if r < fn() else 0
                case T.LessEqualsS: r = 1 if r <= fn() else 0
                case T.Equals: r = int(r == fn())
                case T.NotEquals: r = int(r != fn())
                case _: self.die("AUGH")
        return r

    def kc_binary(self, fn, *toks):
        fn()
        while t := self.match(*toks).type:
            match t:
                case T.BitwiseShiftRight:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x59) # POP CX
                    self.emit8(0x91) # XCHG AX, CX
                    self.emit8(0xD3)
                    self.emit8(self.modrm(0b11, 5, 0)) # SHR AX, CL
                case T.BitwiseShiftLeft:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x59) # POP CX
                    self.emit8(0x91) # XCHG AX, CX
                    self.emit8(0xD3)
                    self.emit8(self.modrm(0b11, 4, 0)) # SHL AX, CL
                case T.BitwiseXor: self.die("AA")
                case T.BitwiseOr:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x0B)
                    self.emit8(self.modrm(0b11, 0, 2)) # OR AX, DX
                case T.BitwiseAnd:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x23)
                    self.emit8(self.modrm(0b11, 0, 2)) # AND AX, DX
                case T.Plus:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x03)
                    self.emit8(self.modrm(0b11, 0, 2)) # ADD AX, DX
                case T.Minus:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x2B)
                    self.emit8(self.modrm(0b11, 2, 0)) # SUB DX, AX
                    self.emit8(0x92) # XCHG AX, DX
                case T.ModuloU:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x59) # POP CX
                    self.emit8(0x91) # XCHG AX, CX
                    self.emit8(0xBA)
                    self.emit16(0) # MOV DX, 0
                    self.emit8(0xF7)
                    self.emit8(self.modrm(0b11, 6, 1)) # DIV CX
                    self.emit8(0x92) # XCHG AX, DX
                case T.SlashU:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x59) # POP CX
                    self.emit8(0x91) # XCHG AX, CX
                    self.emit8(0xBA)
                    self.emit16(0) # MOV DX, 0
                    self.emit8(0xF7)
                    self.emit8(self.modrm(0b11, 6, 1)) # DIV CX
                case T.StarU:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x59) # POP CX
                    self.emit8(0x91) # XCHG AX, CX
                    self.emit8(0xBA)
                    self.emit16(0) # MOV DX, 0
                    self.emit8(0xF7)
                    self.emit8(self.modrm(0b11, 4, 1)) # MUL CX
                # we dont actually need jumps we can read the falgs register
                # check how a c compiler does it!!
                case T.GreaterThanU:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x3B)
                    self.emit8(self.modrm(0b11, 2, 0)) # CMP DX, AX
                    self.emit8(0x77)
                    self.emit8(5) # JA +5
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(1) # MOV AX, 1
                case T.GreaterThanS:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x3B)
                    self.emit8(self.modrm(0b11, 2, 0)) # CMP DX, AX
                    self.emit8(0x7F)
                    self.emit8(5) # JG +5
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(1) # MOV AX, 1
                case T.GreaterEqualsS:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x3B)
                    self.emit8(self.modrm(0b11, 2, 0)) # CMP DX, AX
                    self.emit8(0x7D)
                    self.emit8(5) # JGE +5
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(1) # MOV AX, 1
                case T.GreaterEqualsU:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x3B)
                    self.emit8(self.modrm(0b11, 2, 0)) # CMP DX, AX
                    self.emit8(0x73)
                    self.emit8(5) # JAE +5
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(1) # MOV AX, 1
                case T.LessThanS:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x3B)
                    self.emit8(self.modrm(0b11, 2, 0)) # CMP DX, AX
                    self.emit8(0x7C)
                    self.emit8(5) # JL +5
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(1) # MOV AX, 1
                case T.LessThanU:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x3B)
                    self.emit8(self.modrm(0b11, 2, 0)) # CMP DX, AX
                    self.emit8(0x72)
                    self.emit8(5) # JB +5
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(1) # MOV AX, 1
                case T.Equals:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x3B)
                    self.emit8(self.modrm(0b11, 2, 0)) # CMP DX, AX
                    self.emit8(0x74)
                    self.emit8(5) # JE +5
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(1) # MOV AX, 1
                case T.NotEquals:
                    self.emit8(0x50) # PUSH AX
                    fn()
                    self.emit8(0x5A) # POP DX
                    self.emit8(0x3B)
                    self.emit8(self.modrm(0b11, 2, 0)) # CMP DX, AX
                    self.emit8(0x75)
                    self.emit8(5) # JNE +5
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(1) # MOV AX, 1
                case T.LogicalAnd:
                    self.emit8(0x85)
                    self.emit8(self.modrm(0b11, 0, 0)) # TEST AX, AX
                    br = self.ip
                    if self.first_pass: lbl = 0
                    else: lbl = self.bruh[self.ip]
                    self.emit8(0x74)
                    self.emit8(lbl - br - 2) # JZ lbl
                    fn()
                    self.emit8(0xEB)
                    self.emit8(3) # JMP SHORT +3
                    self.emit8(0xB8)
                    self.emit16(0) # MOV AX, 0
                    if self.first_pass: self.bruh[br] = self.ip
                case _: self.die("AUGH")

    def binary(self, fn, *toks):
        if self.is_kc_expr: return self.kc_binary(fn, *toks)
        return self.c_binary(fn, *toks)

    def c_unary(self):
        e = 1
        while self.match(T.Minus).type: e *= -1
        return self.primary() * e

    def kc_unary(self):
        match self.peek().type:
            case T.Minus:
                self.advance()
                self.unary()
                self.emit8(0xF7)
                self.emit8(self.modrm(0b11, 3, 0)) # NEG AX
            case T.LogicalNot:
                self.advance()
                self.unary()
                self.emit8(0x85)
                self.emit8(self.modrm(0b11, 0, 0)) # TEST AX, AX
                self.emit8(0x9F) # LAHF
                self.emit8(0x25)
                self.emit16(1 << 14) # AND AX, 1 << 6
            case T.BitwiseAnd:
                self.advance()
                ident = self.consume(T.Identifier).value
                if ident not in self.idents or self.idents[ident].type != I.LocalVariable: self.die("unsupported")
                self.emit8(0x8D)
                self.emit8(self.modrm(2, 0, 0b110))
                self.emit16(self.idents[ident].value) # LEA AX, [var]
            case T.Star:
                self.advance()
                self.unary()
                self.emit8(0x93) # XCHG BX, AX
                self.emit8(0x8B)
                self.emit8(self.modrm(0, 0, 0b111)) # MOV AX, [BX]
            case _: self.primary()

    def unary(self):
        if self.is_kc_expr: return self.kc_unary()
        return self.c_unary()

    def c_primary(self):
        t = self.advance()
        match t.type:
            case T.Number: return t.value
            case T.IpCurrent: return self.ip
            case T.IpOrigin: return self.origin
            case T.Identifier: return 0 if self.first_pass else self.idents[t.value].value
            case T.Dot:
                ident = self.consume(T.Identifier).value
                if self.first_pass: return 0
                return self.idents[self.cur_label + ident].value
            case T.Chars:
                v = 0
                for c in t.value:
                    v <<= 8
                    v += ord(c)
                return v
            case T.LeftParen:
                v = self.expression()
                self.consume(T.RightParen)
                return v
            case x: self.die(f"Unexpected token {x}")

    def kc_primary(self):
        t = self.advance()
        match t.type:
            case T.Identifier:
                if t.value not in self.idents and self.first_pass: x = Identifier(I.Label, 0)
                else: x = self.idents[t.value]
                if self.peek().type == T.LeftParen:
                    return self.call(t.value, True)
                match x.type:
                    case I.LocalVariable:
                        self.emit8(0x8B)
                        self.emit8(self.modrm(2, 0, 0b110))
                        self.emit16(x.value) # MOV AX, [var]
                    case I.Label:
                        self.emit8(0xB8)
                        self.emit16(x.value) # MOV AX, label
                    case x: self.die("ugh")
            case T.Number:
                self.emit8(0xB8)
                self.emit16(t.value) # MOV AX, imm
            case T.Chars:
                v = 0
                for c in t.value:
                    v <<= 8
                    v += ord(c)
                if v > 65535: self.die("chars immediate too big")
                self.emit8(0xB8)
                self.emit16(v) # MOV AX, imm
            case T.String:
                if t.value in self.strs:
                    x = self.strs[t.value]
                else:
                    x = self.strs[t.value] = self.str_off
                    self.str_off += len(t.value) + 4
                if not self.first_pass: x += self.data_begin
                self.emit8(0xB8)
                self.emit16(x) # MOV AX, x
            case T.LeftParen:
                self.expression()
                self.consume(T.RightParen)
            case x: self.die(f"Unexpected token {x}")

        while self.peek().type == T.LeftBracket or self.peek().type == T.LeftBrace:
            if self.match(T.LeftBracket).type:
                self.emit8(0x50) # PUSH AX
                self.expression()
                self.consume(T.RightBracket)
                self.emit8(0x5B) # POP BX
                self.emit8(0x96) # XCHG SI, AX
                self.emit8(0xD1)
                self.emit8(self.modrm(0b11, 4, 6)) # SHL SI, 1
                self.emit8(0x8B)
                self.emit8(self.modrm(0, 0, 0b000)) # MOV AX, [BX+SI]
            elif self.match(T.LeftBrace).type:
                self.emit8(0x50) # PUSH AX
                self.expression()
                self.consume(T.RightBrace)
                self.emit8(0x5B) # POP BX
                self.emit8(0x96) # XCHG SI, AX
                self.emit8(0x8A)
                self.emit8(self.modrm(0, 0, 0b000)) # MOV AL, [BX+SI]
                self.emit8(0xB4)
                self.emit8(0) # MOV AH, 0

    def primary(self):
        if self.is_kc_expr: return self.kc_primary()
        return self.c_primary()

    def emit8(self, byte):
        self.ip += 1
        if self.first_pass: return
        self.outstream.write((byte & 0xff).to_bytes())

    def emit16(self, word):
        self.emit8(word)
        self.emit8(word >> 8)

    def emit32(self, dword):
        self.emit16(dword)
        self.emit16(dword >> 16)

    def end(self):
        return self.tokenizer.end

    def advance(self):
        self.prev = self.cur
        self.cur = self.tokenizer.token()
        if not self.prev: self.die("Unexpected EOF")
        return self.prev

    def peek(self):
        return self.cur or Token(None, None)

    def match(self, *toks):
        t = self.peek().type
        return self.advance() if t in toks else Token(None, None)

    def consume(self, *toks):
        t = self.match(*toks)
        if not t.type: self.die(f"Unexpected token {self.peek()}, expected one of {toks}")
        return t

    def die(self, err):
        raise Exception(f"Parser error at line {self.tokenizer.line}, Column {self.tokenizer.column}: {err}")
