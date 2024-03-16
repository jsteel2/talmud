from tokenizer import T, Token, Tokenizer

class Compiler:
    def __init__(self, instream, outstream):
        self.tokenizer = Tokenizer(instream)
        self.outstream = outstream

    def run(self):
        self.labels = {}
        self.cur_label = ""
        self.first_pass = True
        self.compile()
        self.tokenizer.reset()
        self.first_pass = False
        self.compile()

    def compile(self):
        self.prev = Token(None, None)
        self.cur = Token(None, None)
        self.advance()

        self.ip = self.origin = 0x100

        while not self.end(): self.statement()

    def statement(self):
        t = self.advance()
        match t.type:
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
            case T.Identifier: self.label(t.value)
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
            case x: self.die(f"unexpected token {x}")

    def label(self, ident, glbl=True):
        self.consume(T.Colon)
        if glbl: self.cur_label = ident
        if not self.first_pass: return
        if ident in self.labels: self.die(f"redefined label {ident}")
        self.labels[ident] = self.ip

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
            disp = self.imm16()
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

    def expression(self):
        return self.bitwise()

    def bitwise(self):
        return self.binary(self.term, T.BitwiseShiftRight, T.BitwiseShiftLeft, T.BitwiseXor, T.BitwiseXor, T.BitwiseOr, T.BitwiseAnd)

    def term(self):
        return self.binary(self.factor, T.Plus, T.Minus)

    def factor(self):
        return self.binary(self.unary, T.Modulo, T.Slash, T.Star)

    def binary(self, fn, *toks):
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
                case T.Modulo: r %= fn()
                case T.Slash: r //= fn()
                case T.Star: r *= fn()
        return r

    def unary(self):
        e = 1
        while self.match(T.Minus).type: e *= -1
        return self.primary() * e

    def primary(self):
        t = self.advance()
        match t.type:
            case T.Number: return t.value
            case T.IpCurrent: return self.ip
            case T.IpOrigin: return self.origin
            case T.Identifier: return 0 if self.first_pass else self.labels[t.value]
            case T.Dot:
                ident = self.consume(T.Identifier).value
                if self.first_pass: return 0
                return self.labels[self.cur_label + ident]
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
