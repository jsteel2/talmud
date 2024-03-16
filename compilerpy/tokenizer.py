from enum import Enum, auto

class T(Enum):
    Number = 0
    Identifier = auto()
    Chars = auto()
    String = auto()

    Org = auto()
    Db = auto()
    Dw = auto()
    Dd = auto()
    Rb = auto()
    Pad = auto()

    Byte = auto()
    Word = auto()
    Far = auto()

    Add = auto()
    Or = auto()
    Adc = auto()
    Sbb = auto()
    And = auto()
    Sub = auto()
    Xor = auto()
    Cmp = auto()
    Daa = auto()
    Aaa = auto()
    Nop = auto()
    Movsb = auto()
    Movsw = auto()
    Cmpsb = auto()
    Cmpsw = auto()
    Ret = auto()
    Xlatb = auto()
    Lock = auto()
    Repnz = auto()
    Repz = auto()
    Hlt = auto()
    Cmc = auto()
    Das = auto()
    Aas = auto()
    Cbw = auto()
    Cwd = auto()
    Wait = auto()
    Pushf = auto()
    Popf = auto()
    Sahf = auto()
    Lahf = auto()
    Stosb = auto()
    Stosw = auto()
    Lodsb = auto()
    Lodsw = auto()
    Scasb = auto()
    Scasw = auto()
    Retf = auto()
    Into = auto()
    Iret = auto()
    Clc = auto()
    Stc = auto()
    Cli = auto()
    Sti = auto()
    Cld = auto()
    Std = auto()
    Rol = auto()
    Ror = auto()
    Rcl = auto()
    Rcr = auto()
    Shl = auto()
    Shr = auto()
    Sar = auto()
    Not = auto()
    Neg = auto()
    Mul = auto()
    Imul = auto()
    Div = auto()
    Idiv = auto()
    Jmp = auto()
    Call = auto()
    Jo = auto()
    Jno = auto()
    Jb = auto()
    Jnb = auto()
    Jz = auto()
    Jnz = auto()
    Jbe = auto()
    Ja = auto()
    Js = auto()
    Jns = auto()
    Jpe = auto()
    Jpo = auto()
    Jl = auto()
    Jge = auto()
    Jle = auto()
    Jg = auto()
    Loopnz = auto()
    Loopz = auto()
    Loop = auto()
    Jcxz = auto()
    Mov = auto()
    Int = auto()
    Inc = auto()
    Dec = auto()
    Test = auto()
    Lea = auto()
    Les = auto()
    Lds = auto()
    Push = auto()
    Pop = auto()
    Xchg = auto()

    Al = auto()
    Cl = auto()
    Dl = auto()
    Bl = auto()
    Ah = auto()
    Ch = auto()
    Dh = auto()
    Bh = auto()

    Ax = auto()
    Cx = auto()
    Dx = auto()
    Bx = auto()
    Sp = auto()
    Bp = auto()
    Si = auto()
    Di = auto()

    Es = auto()
    Cs = auto()
    Ss = auto()
    Ds = auto()

    LeftParen = auto()
    RightParen = auto()
    LeftBracket = auto()
    RightBracket = auto()
    BitwiseShiftRight = auto()
    BitwiseShiftLeft = auto()
    BitwiseXor = auto()
    BitwiseOr = auto()
    BitwiseAnd = auto()
    Plus = auto()
    Minus = auto()
    Modulo = auto()
    Slash = auto()
    Star = auto()
    Comma = auto()
    Dot = auto()
    Colon = auto()
    IpOrigin = auto()
    IpCurrent = auto()

class Token:
    def __init__(self, type, value=None):
        self.type = type
        self.value = value

    def __repr__(self):
        return f"Token({self.type}, {self.value})"

class Tokenizer:
    def __init__(self, instream):
        self.instream = instream
        self.reset()

    def reset(self):
        self.end = False
        self.char = " "
        self.line = 1
        self.column = 1
        self.instream.seek(0)

    def number_token(self):
        s = self.char
        while not self.advance() and self.char.isalnum():
            s += self.char
        return Token(T.Number, int(s, 0))

    def string_token(self, type, delim):
        s = ""
        while not self.advance() and self.char != delim:
            s += self.char
        if self.char != delim: self.die("unterminated string/char literal")
        self.advance()
        return Token(type, s)

    def ident_token(self):
        ident = self.char
        while not self.advance() and (self.char.isalnum() or self.char == "_"):
            ident += self.char

        match ident:
            case "ORG": return Token(T.Org, ident)
            case "DB": return Token(T.Db, ident)
            case "DW": return Token(T.Dw, ident)
            case "DD": return Token(T.Dd, ident)
            case "RB": return Token(T.Rb, ident)
            case "PAD": return Token(T.Pad, ident)
            case "BYTE": return Token(T.Byte, ident)
            case "WORD": return Token(T.Word, ident)
            case "FAR": return Token(T.Far, ident)
            case "ADD": return Token(T.Add, ident)
            case "ADC": return Token(T.Adc, ident)
            case "AND": return Token(T.And, ident)
            case "XOR": return Token(T.Xor, ident)
            case "OR": return Token(T.Or, ident)
            case "SBB": return Token(T.Sbb, ident)
            case "SUB": return Token(T.Sub, ident)
            case "CMP": return Token(T.Cmp, ident)
            case "DAA": return Token(T.Daa, ident)
            case "AAA": return Token(T.Aaa, ident)
            case "NOP": return Token(T.Nop, ident)
            case "MOVSB": return Token(T.Movsb, ident)
            case "MOVSW": return Token(T.Movsw, ident)
            case "CMPSB": return Token(T.Cmpsb, ident)
            case "CMPSW": return Token(T.Cmpsw, ident)
            case "RET": return Token(T.Ret, ident)
            case "XLATB": return Token(T.Xlatb, ident)
            case "LOCK": return Token(T.Lock, ident)
            case "REPNZ": return Token(T.Repnz, ident)
            case "REPZ": return Token(T.Repz, ident)
            case "HLT": return Token(T.Hlt, ident)
            case "CMC": return Token(T.Cmc, ident)
            case "DAS": return Token(T.Das, ident)
            case "AAS": return Token(T.Aas, ident)
            case "CBW": return Token(T.Cbw, ident)
            case "CWD": return Token(T.Cwd, ident)
            case "WAIT": return Token(T.Wait, ident)
            case "PUSHF": return Token(T.Pushf, ident)
            case "POPF": return Token(T.Popf, ident)
            case "XCHG": return Token(T.Xchg, ident)
            case "SAHF": return Token(T.Sahf, ident)
            case "LAHF": return Token(T.Lahf, ident)
            case "STOSB": return Token(T.Stosb, ident)
            case "STOSW": return Token(T.Stosw, ident)
            case "LODSB": return Token(T.Lodsb, ident)
            case "LODSW": return Token(T.Lodsw, ident)
            case "SCASB": return Token(T.Scasb, ident)
            case "SCASW": return Token(T.Scasw, ident)
            case "RETF": return Token(T.Retf, ident)
            case "INTO": return Token(T.Into, ident)
            case "IRET": return Token(T.Iret, ident)
            case "CLC": return Token(T.Clc, ident)
            case "STC": return Token(T.Stc, ident)
            case "CLI": return Token(T.Cli, ident)
            case "STI": return Token(T.Sti, ident)
            case "CLD": return Token(T.Cld, ident)
            case "STD": return Token(T.Std, ident)
            case "ROL": return Token(T.Rol, ident)
            case "ROR": return Token(T.Ror, ident)
            case "RCL": return Token(T.Rcl, ident)
            case "RCR": return Token(T.Rcr, ident)
            case "SHR": return Token(T.Shr, ident)
            case "SHL": return Token(T.Shl, ident)
            case "SAR": return Token(T.Sar, ident)
            case "NOT": return Token(T.Not, ident)
            case "NEG": return Token(T.Neg, ident)
            case "MUL": return Token(T.Mul, ident)
            case "IMUL": return Token(T.Imul, ident)
            case "DIV": return Token(T.Div, ident)
            case "IDIV": return Token(T.Idiv, ident)
            case "JMP": return Token(T.Jmp, ident)
            case "CALL": return Token(T.Call, ident)
            case "JO": return Token(T.Jo, ident)
            case "JNO": return Token(T.Jno, ident)
            case "JB": return Token(T.Jb, ident)
            case "JNB": return Token(T.Jnb, ident)
            case "JZ": return Token(T.Jz, ident)
            case "JNZ": return Token(T.Jnz, ident)
            case "JBE": return Token(T.Jbe, ident)
            case "JA": return Token(T.Ja, ident)
            case "JS": return Token(T.Js, ident)
            case "JNS": return Token(T.Jns, ident)
            case "JPE": return Token(T.Jpe, ident)
            case "JPO": return Token(T.Jpo, ident)
            case "JL": return Token(T.Jl, ident)
            case "JGE": return Token(T.Jge, ident)
            case "JLE": return Token(T.Jle, ident)
            case "JG": return Token(T.Jg, ident)
            case "LOOPNZ": return Token(T.Loopnz, ident)
            case "LOOPZ": return Token(T.Loopz, ident)
            case "LOOP": return Token(T.Loop, ident)
            case "JCXZ": return Token(T.Jcxz, ident)
            case "MOV": return Token(T.Mov, ident)
            case "INT": return Token(T.Int, ident)
            case "INC": return Token(T.Inc, ident)
            case "DEC": return Token(T.Dec, ident)
            case "TEST": return Token(T.Test, ident)
            case "LEA": return Token(T.Lea, ident)
            case "LES": return Token(T.Les, ident)
            case "LDS": return Token(T.Lds, ident)
            case "PUSH": return Token(T.Push, ident)
            case "POP": return Token(T.Pop, ident)
            case "AL": return Token(T.Al, ident)
            case "CL": return Token(T.Cl, ident)
            case "DL": return Token(T.Dl, ident)
            case "BL": return Token(T.Bl, ident)
            case "AH": return Token(T.Ah, ident)
            case "CH": return Token(T.Ch, ident)
            case "DH": return Token(T.Dh, ident)
            case "BH": return Token(T.Bh, ident)
            case "AX": return Token(T.Ax, ident)
            case "CX": return Token(T.Cx, ident)
            case "DX": return Token(T.Dx, ident)
            case "BX": return Token(T.Bx, ident)
            case "SP": return Token(T.Sp, ident)
            case "BP": return Token(T.Bp, ident)
            case "SI": return Token(T.Si, ident)
            case "DI": return Token(T.Di, ident)
            case "ES": return Token(T.Es, ident)
            case "CS": return Token(T.Cs, ident)
            case "SS": return Token(T.Ss, ident)
            case "DS": return Token(T.Ds, ident)
            case _: return Token(T.Identifier, ident)

    def symbol_token(self):
        def ta(x):
            self.advance()
            return Token(x)

        match self.char:
            case "(": return ta(T.LeftParen)
            case ")": return ta(T.RightParen)
            case "[": return ta(T.LeftBracket)
            case "]": return ta(T.RightBracket)
            case ",": return ta(T.Comma)
            case ".": return ta(T.Dot)
            case "^": return ta(T.BitwiseXor)
            case "|": return ta(T.BitwiseOr)
            case "&": return ta(T.BitwiseAnd)
            case "+": return ta(T.Plus)
            case "-": return ta(T.Minus)
            case "%": return ta(T.Modulo)
            case "/": return ta(T.Slash)
            case "*": return ta(T.Star)
            case ":": return ta(T.Colon)

            case "$":
                if not self.advance() and self.char == "$": return ta(T.IpOrigin)
                else: return Token(T.IpCurrent)
            case ">":
                if not self.advance() and self.char == ">": return ta(T.BitwiseShiftRight)
                else: return Token(T.GreaterThan)
            case "<":
                if not self.advance() and self.char == "<": return ta(T.BitwiseShiftLeft)
                else: return Token(T.LessThan)

            case _: self.die(f"Unknown symbol {self.char}")

    def token(self):
        if self.end: return None

        while self.char.isspace() or self.char == "#":
            if self.char == "#":
                while self.char != "\n":
                    if self.advance(): return None
            else:
                if self.advance(): return None

        if self.char.isnumeric():
            return self.number_token()

        if self.char.isalpha() or self.char == "_":
            return self.ident_token()

        if self.char == "'":
            return self.string_token(T.Chars, "'")

        if self.char == '"':
            return self.string_token(T.String, '"')

        return self.symbol_token()

    def advance(self):
        if self.end: return self.end
        self.char = self.instream.read(1)
        if self.char == "":
            self.end = True
        elif self.char == "\n":
            self.line += 1
            self.column = 1
        else:
            self.column += 1
        return self.end

    def die(self, err):
        raise Exception(f"Tokenizer error At line {self.line}, Column {self.column}: {err}")
