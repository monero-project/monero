#ifdef __APPLE__
#   define ALIGN(x) .align 6
#else
#   define ALIGN(x) .align 64
#endif
.intel_syntax noprefix
#ifdef __APPLE__
#   define FN_PREFIX(fn) _ ## fn
.text
#else
#   define FN_PREFIX(fn) fn
.section .text
#endif

#define PUBLIC .global

PUBLIC FN_PREFIX(CryptonightR_instruction0)
PUBLIC FN_PREFIX(CryptonightR_instruction1)
PUBLIC FN_PREFIX(CryptonightR_instruction2)
PUBLIC FN_PREFIX(CryptonightR_instruction3)
PUBLIC FN_PREFIX(CryptonightR_instruction4)
PUBLIC FN_PREFIX(CryptonightR_instruction5)
PUBLIC FN_PREFIX(CryptonightR_instruction6)
PUBLIC FN_PREFIX(CryptonightR_instruction7)
PUBLIC FN_PREFIX(CryptonightR_instruction8)
PUBLIC FN_PREFIX(CryptonightR_instruction9)
PUBLIC FN_PREFIX(CryptonightR_instruction10)
PUBLIC FN_PREFIX(CryptonightR_instruction11)
PUBLIC FN_PREFIX(CryptonightR_instruction12)
PUBLIC FN_PREFIX(CryptonightR_instruction13)
PUBLIC FN_PREFIX(CryptonightR_instruction14)
PUBLIC FN_PREFIX(CryptonightR_instruction15)
PUBLIC FN_PREFIX(CryptonightR_instruction16)
PUBLIC FN_PREFIX(CryptonightR_instruction17)
PUBLIC FN_PREFIX(CryptonightR_instruction18)
PUBLIC FN_PREFIX(CryptonightR_instruction19)
PUBLIC FN_PREFIX(CryptonightR_instruction20)
PUBLIC FN_PREFIX(CryptonightR_instruction21)
PUBLIC FN_PREFIX(CryptonightR_instruction22)
PUBLIC FN_PREFIX(CryptonightR_instruction23)
PUBLIC FN_PREFIX(CryptonightR_instruction24)
PUBLIC FN_PREFIX(CryptonightR_instruction25)
PUBLIC FN_PREFIX(CryptonightR_instruction26)
PUBLIC FN_PREFIX(CryptonightR_instruction27)
PUBLIC FN_PREFIX(CryptonightR_instruction28)
PUBLIC FN_PREFIX(CryptonightR_instruction29)
PUBLIC FN_PREFIX(CryptonightR_instruction30)
PUBLIC FN_PREFIX(CryptonightR_instruction31)
PUBLIC FN_PREFIX(CryptonightR_instruction32)
PUBLIC FN_PREFIX(CryptonightR_instruction33)
PUBLIC FN_PREFIX(CryptonightR_instruction34)
PUBLIC FN_PREFIX(CryptonightR_instruction35)
PUBLIC FN_PREFIX(CryptonightR_instruction36)
PUBLIC FN_PREFIX(CryptonightR_instruction37)
PUBLIC FN_PREFIX(CryptonightR_instruction38)
PUBLIC FN_PREFIX(CryptonightR_instruction39)
PUBLIC FN_PREFIX(CryptonightR_instruction40)
PUBLIC FN_PREFIX(CryptonightR_instruction41)
PUBLIC FN_PREFIX(CryptonightR_instruction42)
PUBLIC FN_PREFIX(CryptonightR_instruction43)
PUBLIC FN_PREFIX(CryptonightR_instruction44)
PUBLIC FN_PREFIX(CryptonightR_instruction45)
PUBLIC FN_PREFIX(CryptonightR_instruction46)
PUBLIC FN_PREFIX(CryptonightR_instruction47)
PUBLIC FN_PREFIX(CryptonightR_instruction48)
PUBLIC FN_PREFIX(CryptonightR_instruction49)
PUBLIC FN_PREFIX(CryptonightR_instruction50)
PUBLIC FN_PREFIX(CryptonightR_instruction51)
PUBLIC FN_PREFIX(CryptonightR_instruction52)
PUBLIC FN_PREFIX(CryptonightR_instruction53)
PUBLIC FN_PREFIX(CryptonightR_instruction54)
PUBLIC FN_PREFIX(CryptonightR_instruction55)
PUBLIC FN_PREFIX(CryptonightR_instruction56)
PUBLIC FN_PREFIX(CryptonightR_instruction57)
PUBLIC FN_PREFIX(CryptonightR_instruction58)
PUBLIC FN_PREFIX(CryptonightR_instruction59)
PUBLIC FN_PREFIX(CryptonightR_instruction60)
PUBLIC FN_PREFIX(CryptonightR_instruction61)
PUBLIC FN_PREFIX(CryptonightR_instruction62)
PUBLIC FN_PREFIX(CryptonightR_instruction63)
PUBLIC FN_PREFIX(CryptonightR_instruction64)
PUBLIC FN_PREFIX(CryptonightR_instruction65)
PUBLIC FN_PREFIX(CryptonightR_instruction66)
PUBLIC FN_PREFIX(CryptonightR_instruction67)
PUBLIC FN_PREFIX(CryptonightR_instruction68)
PUBLIC FN_PREFIX(CryptonightR_instruction69)
PUBLIC FN_PREFIX(CryptonightR_instruction70)
PUBLIC FN_PREFIX(CryptonightR_instruction71)
PUBLIC FN_PREFIX(CryptonightR_instruction72)
PUBLIC FN_PREFIX(CryptonightR_instruction73)
PUBLIC FN_PREFIX(CryptonightR_instruction74)
PUBLIC FN_PREFIX(CryptonightR_instruction75)
PUBLIC FN_PREFIX(CryptonightR_instruction76)
PUBLIC FN_PREFIX(CryptonightR_instruction77)
PUBLIC FN_PREFIX(CryptonightR_instruction78)
PUBLIC FN_PREFIX(CryptonightR_instruction79)
PUBLIC FN_PREFIX(CryptonightR_instruction80)
PUBLIC FN_PREFIX(CryptonightR_instruction81)
PUBLIC FN_PREFIX(CryptonightR_instruction82)
PUBLIC FN_PREFIX(CryptonightR_instruction83)
PUBLIC FN_PREFIX(CryptonightR_instruction84)
PUBLIC FN_PREFIX(CryptonightR_instruction85)
PUBLIC FN_PREFIX(CryptonightR_instruction86)
PUBLIC FN_PREFIX(CryptonightR_instruction87)
PUBLIC FN_PREFIX(CryptonightR_instruction88)
PUBLIC FN_PREFIX(CryptonightR_instruction89)
PUBLIC FN_PREFIX(CryptonightR_instruction90)
PUBLIC FN_PREFIX(CryptonightR_instruction91)
PUBLIC FN_PREFIX(CryptonightR_instruction92)
PUBLIC FN_PREFIX(CryptonightR_instruction93)
PUBLIC FN_PREFIX(CryptonightR_instruction94)
PUBLIC FN_PREFIX(CryptonightR_instruction95)
PUBLIC FN_PREFIX(CryptonightR_instruction96)
PUBLIC FN_PREFIX(CryptonightR_instruction97)
PUBLIC FN_PREFIX(CryptonightR_instruction98)
PUBLIC FN_PREFIX(CryptonightR_instruction99)
PUBLIC FN_PREFIX(CryptonightR_instruction100)
PUBLIC FN_PREFIX(CryptonightR_instruction101)
PUBLIC FN_PREFIX(CryptonightR_instruction102)
PUBLIC FN_PREFIX(CryptonightR_instruction103)
PUBLIC FN_PREFIX(CryptonightR_instruction104)
PUBLIC FN_PREFIX(CryptonightR_instruction105)
PUBLIC FN_PREFIX(CryptonightR_instruction106)
PUBLIC FN_PREFIX(CryptonightR_instruction107)
PUBLIC FN_PREFIX(CryptonightR_instruction108)
PUBLIC FN_PREFIX(CryptonightR_instruction109)
PUBLIC FN_PREFIX(CryptonightR_instruction110)
PUBLIC FN_PREFIX(CryptonightR_instruction111)
PUBLIC FN_PREFIX(CryptonightR_instruction112)
PUBLIC FN_PREFIX(CryptonightR_instruction113)
PUBLIC FN_PREFIX(CryptonightR_instruction114)
PUBLIC FN_PREFIX(CryptonightR_instruction115)
PUBLIC FN_PREFIX(CryptonightR_instruction116)
PUBLIC FN_PREFIX(CryptonightR_instruction117)
PUBLIC FN_PREFIX(CryptonightR_instruction118)
PUBLIC FN_PREFIX(CryptonightR_instruction119)
PUBLIC FN_PREFIX(CryptonightR_instruction120)
PUBLIC FN_PREFIX(CryptonightR_instruction121)
PUBLIC FN_PREFIX(CryptonightR_instruction122)
PUBLIC FN_PREFIX(CryptonightR_instruction123)
PUBLIC FN_PREFIX(CryptonightR_instruction124)
PUBLIC FN_PREFIX(CryptonightR_instruction125)
PUBLIC FN_PREFIX(CryptonightR_instruction126)
PUBLIC FN_PREFIX(CryptonightR_instruction127)
PUBLIC FN_PREFIX(CryptonightR_instruction128)
PUBLIC FN_PREFIX(CryptonightR_instruction129)
PUBLIC FN_PREFIX(CryptonightR_instruction130)
PUBLIC FN_PREFIX(CryptonightR_instruction131)
PUBLIC FN_PREFIX(CryptonightR_instruction132)
PUBLIC FN_PREFIX(CryptonightR_instruction133)
PUBLIC FN_PREFIX(CryptonightR_instruction134)
PUBLIC FN_PREFIX(CryptonightR_instruction135)
PUBLIC FN_PREFIX(CryptonightR_instruction136)
PUBLIC FN_PREFIX(CryptonightR_instruction137)
PUBLIC FN_PREFIX(CryptonightR_instruction138)
PUBLIC FN_PREFIX(CryptonightR_instruction139)
PUBLIC FN_PREFIX(CryptonightR_instruction140)
PUBLIC FN_PREFIX(CryptonightR_instruction141)
PUBLIC FN_PREFIX(CryptonightR_instruction142)
PUBLIC FN_PREFIX(CryptonightR_instruction143)
PUBLIC FN_PREFIX(CryptonightR_instruction144)
PUBLIC FN_PREFIX(CryptonightR_instruction145)
PUBLIC FN_PREFIX(CryptonightR_instruction146)
PUBLIC FN_PREFIX(CryptonightR_instruction147)
PUBLIC FN_PREFIX(CryptonightR_instruction148)
PUBLIC FN_PREFIX(CryptonightR_instruction149)
PUBLIC FN_PREFIX(CryptonightR_instruction150)
PUBLIC FN_PREFIX(CryptonightR_instruction151)
PUBLIC FN_PREFIX(CryptonightR_instruction152)
PUBLIC FN_PREFIX(CryptonightR_instruction153)
PUBLIC FN_PREFIX(CryptonightR_instruction154)
PUBLIC FN_PREFIX(CryptonightR_instruction155)
PUBLIC FN_PREFIX(CryptonightR_instruction156)
PUBLIC FN_PREFIX(CryptonightR_instruction157)
PUBLIC FN_PREFIX(CryptonightR_instruction158)
PUBLIC FN_PREFIX(CryptonightR_instruction159)
PUBLIC FN_PREFIX(CryptonightR_instruction160)
PUBLIC FN_PREFIX(CryptonightR_instruction161)
PUBLIC FN_PREFIX(CryptonightR_instruction162)
PUBLIC FN_PREFIX(CryptonightR_instruction163)
PUBLIC FN_PREFIX(CryptonightR_instruction164)
PUBLIC FN_PREFIX(CryptonightR_instruction165)
PUBLIC FN_PREFIX(CryptonightR_instruction166)
PUBLIC FN_PREFIX(CryptonightR_instruction167)
PUBLIC FN_PREFIX(CryptonightR_instruction168)
PUBLIC FN_PREFIX(CryptonightR_instruction169)
PUBLIC FN_PREFIX(CryptonightR_instruction170)
PUBLIC FN_PREFIX(CryptonightR_instruction171)
PUBLIC FN_PREFIX(CryptonightR_instruction172)
PUBLIC FN_PREFIX(CryptonightR_instruction173)
PUBLIC FN_PREFIX(CryptonightR_instruction174)
PUBLIC FN_PREFIX(CryptonightR_instruction175)
PUBLIC FN_PREFIX(CryptonightR_instruction176)
PUBLIC FN_PREFIX(CryptonightR_instruction177)
PUBLIC FN_PREFIX(CryptonightR_instruction178)
PUBLIC FN_PREFIX(CryptonightR_instruction179)
PUBLIC FN_PREFIX(CryptonightR_instruction180)
PUBLIC FN_PREFIX(CryptonightR_instruction181)
PUBLIC FN_PREFIX(CryptonightR_instruction182)
PUBLIC FN_PREFIX(CryptonightR_instruction183)
PUBLIC FN_PREFIX(CryptonightR_instruction184)
PUBLIC FN_PREFIX(CryptonightR_instruction185)
PUBLIC FN_PREFIX(CryptonightR_instruction186)
PUBLIC FN_PREFIX(CryptonightR_instruction187)
PUBLIC FN_PREFIX(CryptonightR_instruction188)
PUBLIC FN_PREFIX(CryptonightR_instruction189)
PUBLIC FN_PREFIX(CryptonightR_instruction190)
PUBLIC FN_PREFIX(CryptonightR_instruction191)
PUBLIC FN_PREFIX(CryptonightR_instruction192)
PUBLIC FN_PREFIX(CryptonightR_instruction193)
PUBLIC FN_PREFIX(CryptonightR_instruction194)
PUBLIC FN_PREFIX(CryptonightR_instruction195)
PUBLIC FN_PREFIX(CryptonightR_instruction196)
PUBLIC FN_PREFIX(CryptonightR_instruction197)
PUBLIC FN_PREFIX(CryptonightR_instruction198)
PUBLIC FN_PREFIX(CryptonightR_instruction199)
PUBLIC FN_PREFIX(CryptonightR_instruction200)
PUBLIC FN_PREFIX(CryptonightR_instruction201)
PUBLIC FN_PREFIX(CryptonightR_instruction202)
PUBLIC FN_PREFIX(CryptonightR_instruction203)
PUBLIC FN_PREFIX(CryptonightR_instruction204)
PUBLIC FN_PREFIX(CryptonightR_instruction205)
PUBLIC FN_PREFIX(CryptonightR_instruction206)
PUBLIC FN_PREFIX(CryptonightR_instruction207)
PUBLIC FN_PREFIX(CryptonightR_instruction208)
PUBLIC FN_PREFIX(CryptonightR_instruction209)
PUBLIC FN_PREFIX(CryptonightR_instruction210)
PUBLIC FN_PREFIX(CryptonightR_instruction211)
PUBLIC FN_PREFIX(CryptonightR_instruction212)
PUBLIC FN_PREFIX(CryptonightR_instruction213)
PUBLIC FN_PREFIX(CryptonightR_instruction214)
PUBLIC FN_PREFIX(CryptonightR_instruction215)
PUBLIC FN_PREFIX(CryptonightR_instruction216)
PUBLIC FN_PREFIX(CryptonightR_instruction217)
PUBLIC FN_PREFIX(CryptonightR_instruction218)
PUBLIC FN_PREFIX(CryptonightR_instruction219)
PUBLIC FN_PREFIX(CryptonightR_instruction220)
PUBLIC FN_PREFIX(CryptonightR_instruction221)
PUBLIC FN_PREFIX(CryptonightR_instruction222)
PUBLIC FN_PREFIX(CryptonightR_instruction223)
PUBLIC FN_PREFIX(CryptonightR_instruction224)
PUBLIC FN_PREFIX(CryptonightR_instruction225)
PUBLIC FN_PREFIX(CryptonightR_instruction226)
PUBLIC FN_PREFIX(CryptonightR_instruction227)
PUBLIC FN_PREFIX(CryptonightR_instruction228)
PUBLIC FN_PREFIX(CryptonightR_instruction229)
PUBLIC FN_PREFIX(CryptonightR_instruction230)
PUBLIC FN_PREFIX(CryptonightR_instruction231)
PUBLIC FN_PREFIX(CryptonightR_instruction232)
PUBLIC FN_PREFIX(CryptonightR_instruction233)
PUBLIC FN_PREFIX(CryptonightR_instruction234)
PUBLIC FN_PREFIX(CryptonightR_instruction235)
PUBLIC FN_PREFIX(CryptonightR_instruction236)
PUBLIC FN_PREFIX(CryptonightR_instruction237)
PUBLIC FN_PREFIX(CryptonightR_instruction238)
PUBLIC FN_PREFIX(CryptonightR_instruction239)
PUBLIC FN_PREFIX(CryptonightR_instruction240)
PUBLIC FN_PREFIX(CryptonightR_instruction241)
PUBLIC FN_PREFIX(CryptonightR_instruction242)
PUBLIC FN_PREFIX(CryptonightR_instruction243)
PUBLIC FN_PREFIX(CryptonightR_instruction244)
PUBLIC FN_PREFIX(CryptonightR_instruction245)
PUBLIC FN_PREFIX(CryptonightR_instruction246)
PUBLIC FN_PREFIX(CryptonightR_instruction247)
PUBLIC FN_PREFIX(CryptonightR_instruction248)
PUBLIC FN_PREFIX(CryptonightR_instruction249)
PUBLIC FN_PREFIX(CryptonightR_instruction250)
PUBLIC FN_PREFIX(CryptonightR_instruction251)
PUBLIC FN_PREFIX(CryptonightR_instruction252)
PUBLIC FN_PREFIX(CryptonightR_instruction253)
PUBLIC FN_PREFIX(CryptonightR_instruction254)
PUBLIC FN_PREFIX(CryptonightR_instruction255)
PUBLIC FN_PREFIX(CryptonightR_instruction256)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov0)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov1)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov2)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov3)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov4)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov5)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov6)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov7)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov8)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov9)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov10)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov11)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov12)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov13)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov14)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov15)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov16)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov17)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov18)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov19)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov20)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov21)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov22)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov23)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov24)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov25)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov26)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov27)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov28)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov29)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov30)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov31)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov32)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov33)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov34)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov35)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov36)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov37)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov38)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov39)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov40)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov41)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov42)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov43)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov44)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov45)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov46)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov47)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov48)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov49)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov50)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov51)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov52)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov53)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov54)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov55)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov56)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov57)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov58)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov59)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov60)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov61)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov62)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov63)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov64)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov65)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov66)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov67)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov68)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov69)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov70)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov71)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov72)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov73)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov74)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov75)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov76)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov77)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov78)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov79)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov80)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov81)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov82)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov83)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov84)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov85)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov86)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov87)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov88)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov89)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov90)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov91)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov92)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov93)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov94)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov95)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov96)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov97)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov98)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov99)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov100)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov101)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov102)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov103)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov104)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov105)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov106)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov107)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov108)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov109)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov110)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov111)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov112)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov113)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov114)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov115)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov116)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov117)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov118)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov119)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov120)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov121)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov122)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov123)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov124)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov125)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov126)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov127)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov128)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov129)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov130)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov131)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov132)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov133)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov134)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov135)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov136)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov137)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov138)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov139)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov140)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov141)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov142)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov143)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov144)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov145)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov146)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov147)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov148)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov149)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov150)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov151)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov152)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov153)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov154)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov155)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov156)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov157)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov158)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov159)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov160)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov161)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov162)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov163)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov164)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov165)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov166)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov167)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov168)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov169)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov170)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov171)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov172)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov173)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov174)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov175)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov176)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov177)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov178)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov179)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov180)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov181)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov182)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov183)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov184)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov185)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov186)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov187)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov188)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov189)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov190)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov191)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov192)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov193)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov194)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov195)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov196)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov197)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov198)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov199)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov200)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov201)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov202)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov203)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov204)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov205)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov206)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov207)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov208)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov209)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov210)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov211)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov212)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov213)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov214)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov215)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov216)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov217)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov218)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov219)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov220)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov221)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov222)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov223)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov224)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov225)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov226)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov227)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov228)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov229)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov230)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov231)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov232)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov233)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov234)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov235)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov236)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov237)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov238)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov239)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov240)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov241)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov242)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov243)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov244)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov245)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov246)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov247)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov248)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov249)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov250)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov251)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov252)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov253)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov254)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov255)
PUBLIC FN_PREFIX(CryptonightR_instruction_mov256)

FN_PREFIX(CryptonightR_instruction0):
	imul	ebx, ebx
FN_PREFIX(CryptonightR_instruction1):
	imul	ebx, ebx
FN_PREFIX(CryptonightR_instruction2):
	imul	ebx, ebx
FN_PREFIX(CryptonightR_instruction3):
	add	ebx, r9d
	add	ebx, 2147483647
FN_PREFIX(CryptonightR_instruction4):
	sub	ebx, r9d
FN_PREFIX(CryptonightR_instruction5):
	ror	ebx, cl
FN_PREFIX(CryptonightR_instruction6):
	rol	ebx, cl
FN_PREFIX(CryptonightR_instruction7):
	xor	ebx, r9d
FN_PREFIX(CryptonightR_instruction8):
	imul	esi, ebx
FN_PREFIX(CryptonightR_instruction9):
	imul	esi, ebx
FN_PREFIX(CryptonightR_instruction10):
	imul	esi, ebx
FN_PREFIX(CryptonightR_instruction11):
	add	esi, ebx
	add	esi, 2147483647
FN_PREFIX(CryptonightR_instruction12):
	sub	esi, ebx
FN_PREFIX(CryptonightR_instruction13):
	ror	esi, cl
FN_PREFIX(CryptonightR_instruction14):
	rol	esi, cl
FN_PREFIX(CryptonightR_instruction15):
	xor	esi, ebx
FN_PREFIX(CryptonightR_instruction16):
	imul	edi, ebx
FN_PREFIX(CryptonightR_instruction17):
	imul	edi, ebx
FN_PREFIX(CryptonightR_instruction18):
	imul	edi, ebx
FN_PREFIX(CryptonightR_instruction19):
	add	edi, ebx
	add	edi, 2147483647
FN_PREFIX(CryptonightR_instruction20):
	sub	edi, ebx
FN_PREFIX(CryptonightR_instruction21):
	ror	edi, cl
FN_PREFIX(CryptonightR_instruction22):
	rol	edi, cl
FN_PREFIX(CryptonightR_instruction23):
	xor	edi, ebx
FN_PREFIX(CryptonightR_instruction24):
	imul	ebp, ebx
FN_PREFIX(CryptonightR_instruction25):
	imul	ebp, ebx
FN_PREFIX(CryptonightR_instruction26):
	imul	ebp, ebx
FN_PREFIX(CryptonightR_instruction27):
	add	ebp, ebx
	add	ebp, 2147483647
FN_PREFIX(CryptonightR_instruction28):
	sub	ebp, ebx
FN_PREFIX(CryptonightR_instruction29):
	ror	ebp, cl
FN_PREFIX(CryptonightR_instruction30):
	rol	ebp, cl
FN_PREFIX(CryptonightR_instruction31):
	xor	ebp, ebx
FN_PREFIX(CryptonightR_instruction32):
	imul	ebx, esi
FN_PREFIX(CryptonightR_instruction33):
	imul	ebx, esi
FN_PREFIX(CryptonightR_instruction34):
	imul	ebx, esi
FN_PREFIX(CryptonightR_instruction35):
	add	ebx, esi
	add	ebx, 2147483647
FN_PREFIX(CryptonightR_instruction36):
	sub	ebx, esi
FN_PREFIX(CryptonightR_instruction37):
	ror	ebx, cl
FN_PREFIX(CryptonightR_instruction38):
	rol	ebx, cl
FN_PREFIX(CryptonightR_instruction39):
	xor	ebx, esi
FN_PREFIX(CryptonightR_instruction40):
	imul	esi, esi
FN_PREFIX(CryptonightR_instruction41):
	imul	esi, esi
FN_PREFIX(CryptonightR_instruction42):
	imul	esi, esi
FN_PREFIX(CryptonightR_instruction43):
	add	esi, r9d
	add	esi, 2147483647
FN_PREFIX(CryptonightR_instruction44):
	sub	esi, r9d
FN_PREFIX(CryptonightR_instruction45):
	ror	esi, cl
FN_PREFIX(CryptonightR_instruction46):
	rol	esi, cl
FN_PREFIX(CryptonightR_instruction47):
	xor	esi, r9d
FN_PREFIX(CryptonightR_instruction48):
	imul	edi, esi
FN_PREFIX(CryptonightR_instruction49):
	imul	edi, esi
FN_PREFIX(CryptonightR_instruction50):
	imul	edi, esi
FN_PREFIX(CryptonightR_instruction51):
	add	edi, esi
	add	edi, 2147483647
FN_PREFIX(CryptonightR_instruction52):
	sub	edi, esi
FN_PREFIX(CryptonightR_instruction53):
	ror	edi, cl
FN_PREFIX(CryptonightR_instruction54):
	rol	edi, cl
FN_PREFIX(CryptonightR_instruction55):
	xor	edi, esi
FN_PREFIX(CryptonightR_instruction56):
	imul	ebp, esi
FN_PREFIX(CryptonightR_instruction57):
	imul	ebp, esi
FN_PREFIX(CryptonightR_instruction58):
	imul	ebp, esi
FN_PREFIX(CryptonightR_instruction59):
	add	ebp, esi
	add	ebp, 2147483647
FN_PREFIX(CryptonightR_instruction60):
	sub	ebp, esi
FN_PREFIX(CryptonightR_instruction61):
	ror	ebp, cl
FN_PREFIX(CryptonightR_instruction62):
	rol	ebp, cl
FN_PREFIX(CryptonightR_instruction63):
	xor	ebp, esi
FN_PREFIX(CryptonightR_instruction64):
	imul	ebx, edi
FN_PREFIX(CryptonightR_instruction65):
	imul	ebx, edi
FN_PREFIX(CryptonightR_instruction66):
	imul	ebx, edi
FN_PREFIX(CryptonightR_instruction67):
	add	ebx, edi
	add	ebx, 2147483647
FN_PREFIX(CryptonightR_instruction68):
	sub	ebx, edi
FN_PREFIX(CryptonightR_instruction69):
	ror	ebx, cl
FN_PREFIX(CryptonightR_instruction70):
	rol	ebx, cl
FN_PREFIX(CryptonightR_instruction71):
	xor	ebx, edi
FN_PREFIX(CryptonightR_instruction72):
	imul	esi, edi
FN_PREFIX(CryptonightR_instruction73):
	imul	esi, edi
FN_PREFIX(CryptonightR_instruction74):
	imul	esi, edi
FN_PREFIX(CryptonightR_instruction75):
	add	esi, edi
	add	esi, 2147483647
FN_PREFIX(CryptonightR_instruction76):
	sub	esi, edi
FN_PREFIX(CryptonightR_instruction77):
	ror	esi, cl
FN_PREFIX(CryptonightR_instruction78):
	rol	esi, cl
FN_PREFIX(CryptonightR_instruction79):
	xor	esi, edi
FN_PREFIX(CryptonightR_instruction80):
	imul	edi, edi
FN_PREFIX(CryptonightR_instruction81):
	imul	edi, edi
FN_PREFIX(CryptonightR_instruction82):
	imul	edi, edi
FN_PREFIX(CryptonightR_instruction83):
	add	edi, r9d
	add	edi, 2147483647
FN_PREFIX(CryptonightR_instruction84):
	sub	edi, r9d
FN_PREFIX(CryptonightR_instruction85):
	ror	edi, cl
FN_PREFIX(CryptonightR_instruction86):
	rol	edi, cl
FN_PREFIX(CryptonightR_instruction87):
	xor	edi, r9d
FN_PREFIX(CryptonightR_instruction88):
	imul	ebp, edi
FN_PREFIX(CryptonightR_instruction89):
	imul	ebp, edi
FN_PREFIX(CryptonightR_instruction90):
	imul	ebp, edi
FN_PREFIX(CryptonightR_instruction91):
	add	ebp, edi
	add	ebp, 2147483647
FN_PREFIX(CryptonightR_instruction92):
	sub	ebp, edi
FN_PREFIX(CryptonightR_instruction93):
	ror	ebp, cl
FN_PREFIX(CryptonightR_instruction94):
	rol	ebp, cl
FN_PREFIX(CryptonightR_instruction95):
	xor	ebp, edi
FN_PREFIX(CryptonightR_instruction96):
	imul	ebx, ebp
FN_PREFIX(CryptonightR_instruction97):
	imul	ebx, ebp
FN_PREFIX(CryptonightR_instruction98):
	imul	ebx, ebp
FN_PREFIX(CryptonightR_instruction99):
	add	ebx, ebp
	add	ebx, 2147483647
FN_PREFIX(CryptonightR_instruction100):
	sub	ebx, ebp
FN_PREFIX(CryptonightR_instruction101):
	ror	ebx, cl
FN_PREFIX(CryptonightR_instruction102):
	rol	ebx, cl
FN_PREFIX(CryptonightR_instruction103):
	xor	ebx, ebp
FN_PREFIX(CryptonightR_instruction104):
	imul	esi, ebp
FN_PREFIX(CryptonightR_instruction105):
	imul	esi, ebp
FN_PREFIX(CryptonightR_instruction106):
	imul	esi, ebp
FN_PREFIX(CryptonightR_instruction107):
	add	esi, ebp
	add	esi, 2147483647
FN_PREFIX(CryptonightR_instruction108):
	sub	esi, ebp
FN_PREFIX(CryptonightR_instruction109):
	ror	esi, cl
FN_PREFIX(CryptonightR_instruction110):
	rol	esi, cl
FN_PREFIX(CryptonightR_instruction111):
	xor	esi, ebp
FN_PREFIX(CryptonightR_instruction112):
	imul	edi, ebp
FN_PREFIX(CryptonightR_instruction113):
	imul	edi, ebp
FN_PREFIX(CryptonightR_instruction114):
	imul	edi, ebp
FN_PREFIX(CryptonightR_instruction115):
	add	edi, ebp
	add	edi, 2147483647
FN_PREFIX(CryptonightR_instruction116):
	sub	edi, ebp
FN_PREFIX(CryptonightR_instruction117):
	ror	edi, cl
FN_PREFIX(CryptonightR_instruction118):
	rol	edi, cl
FN_PREFIX(CryptonightR_instruction119):
	xor	edi, ebp
FN_PREFIX(CryptonightR_instruction120):
	imul	ebp, ebp
FN_PREFIX(CryptonightR_instruction121):
	imul	ebp, ebp
FN_PREFIX(CryptonightR_instruction122):
	imul	ebp, ebp
FN_PREFIX(CryptonightR_instruction123):
	add	ebp, r9d
	add	ebp, 2147483647
FN_PREFIX(CryptonightR_instruction124):
	sub	ebp, r9d
FN_PREFIX(CryptonightR_instruction125):
	ror	ebp, cl
FN_PREFIX(CryptonightR_instruction126):
	rol	ebp, cl
FN_PREFIX(CryptonightR_instruction127):
	xor	ebp, r9d
FN_PREFIX(CryptonightR_instruction128):
	imul	ebx, esp
FN_PREFIX(CryptonightR_instruction129):
	imul	ebx, esp
FN_PREFIX(CryptonightR_instruction130):
	imul	ebx, esp
FN_PREFIX(CryptonightR_instruction131):
	add	ebx, esp
	add	ebx, 2147483647
FN_PREFIX(CryptonightR_instruction132):
	sub	ebx, esp
FN_PREFIX(CryptonightR_instruction133):
	ror	ebx, cl
FN_PREFIX(CryptonightR_instruction134):
	rol	ebx, cl
FN_PREFIX(CryptonightR_instruction135):
	xor	ebx, esp
FN_PREFIX(CryptonightR_instruction136):
	imul	esi, esp
FN_PREFIX(CryptonightR_instruction137):
	imul	esi, esp
FN_PREFIX(CryptonightR_instruction138):
	imul	esi, esp
FN_PREFIX(CryptonightR_instruction139):
	add	esi, esp
	add	esi, 2147483647
FN_PREFIX(CryptonightR_instruction140):
	sub	esi, esp
FN_PREFIX(CryptonightR_instruction141):
	ror	esi, cl
FN_PREFIX(CryptonightR_instruction142):
	rol	esi, cl
FN_PREFIX(CryptonightR_instruction143):
	xor	esi, esp
FN_PREFIX(CryptonightR_instruction144):
	imul	edi, esp
FN_PREFIX(CryptonightR_instruction145):
	imul	edi, esp
FN_PREFIX(CryptonightR_instruction146):
	imul	edi, esp
FN_PREFIX(CryptonightR_instruction147):
	add	edi, esp
	add	edi, 2147483647
FN_PREFIX(CryptonightR_instruction148):
	sub	edi, esp
FN_PREFIX(CryptonightR_instruction149):
	ror	edi, cl
FN_PREFIX(CryptonightR_instruction150):
	rol	edi, cl
FN_PREFIX(CryptonightR_instruction151):
	xor	edi, esp
FN_PREFIX(CryptonightR_instruction152):
	imul	ebp, esp
FN_PREFIX(CryptonightR_instruction153):
	imul	ebp, esp
FN_PREFIX(CryptonightR_instruction154):
	imul	ebp, esp
FN_PREFIX(CryptonightR_instruction155):
	add	ebp, esp
	add	ebp, 2147483647
FN_PREFIX(CryptonightR_instruction156):
	sub	ebp, esp
FN_PREFIX(CryptonightR_instruction157):
	ror	ebp, cl
FN_PREFIX(CryptonightR_instruction158):
	rol	ebp, cl
FN_PREFIX(CryptonightR_instruction159):
	xor	ebp, esp
FN_PREFIX(CryptonightR_instruction160):
	imul	ebx, r15d
FN_PREFIX(CryptonightR_instruction161):
	imul	ebx, r15d
FN_PREFIX(CryptonightR_instruction162):
	imul	ebx, r15d
FN_PREFIX(CryptonightR_instruction163):
	add	ebx, r15d
	add	ebx, 2147483647
FN_PREFIX(CryptonightR_instruction164):
	sub	ebx, r15d
FN_PREFIX(CryptonightR_instruction165):
	ror	ebx, cl
FN_PREFIX(CryptonightR_instruction166):
	rol	ebx, cl
FN_PREFIX(CryptonightR_instruction167):
	xor	ebx, r15d
FN_PREFIX(CryptonightR_instruction168):
	imul	esi, r15d
FN_PREFIX(CryptonightR_instruction169):
	imul	esi, r15d
FN_PREFIX(CryptonightR_instruction170):
	imul	esi, r15d
FN_PREFIX(CryptonightR_instruction171):
	add	esi, r15d
	add	esi, 2147483647
FN_PREFIX(CryptonightR_instruction172):
	sub	esi, r15d
FN_PREFIX(CryptonightR_instruction173):
	ror	esi, cl
FN_PREFIX(CryptonightR_instruction174):
	rol	esi, cl
FN_PREFIX(CryptonightR_instruction175):
	xor	esi, r15d
FN_PREFIX(CryptonightR_instruction176):
	imul	edi, r15d
FN_PREFIX(CryptonightR_instruction177):
	imul	edi, r15d
FN_PREFIX(CryptonightR_instruction178):
	imul	edi, r15d
FN_PREFIX(CryptonightR_instruction179):
	add	edi, r15d
	add	edi, 2147483647
FN_PREFIX(CryptonightR_instruction180):
	sub	edi, r15d
FN_PREFIX(CryptonightR_instruction181):
	ror	edi, cl
FN_PREFIX(CryptonightR_instruction182):
	rol	edi, cl
FN_PREFIX(CryptonightR_instruction183):
	xor	edi, r15d
FN_PREFIX(CryptonightR_instruction184):
	imul	ebp, r15d
FN_PREFIX(CryptonightR_instruction185):
	imul	ebp, r15d
FN_PREFIX(CryptonightR_instruction186):
	imul	ebp, r15d
FN_PREFIX(CryptonightR_instruction187):
	add	ebp, r15d
	add	ebp, 2147483647
FN_PREFIX(CryptonightR_instruction188):
	sub	ebp, r15d
FN_PREFIX(CryptonightR_instruction189):
	ror	ebp, cl
FN_PREFIX(CryptonightR_instruction190):
	rol	ebp, cl
FN_PREFIX(CryptonightR_instruction191):
	xor	ebp, r15d
FN_PREFIX(CryptonightR_instruction192):
	imul	ebx, eax
FN_PREFIX(CryptonightR_instruction193):
	imul	ebx, eax
FN_PREFIX(CryptonightR_instruction194):
	imul	ebx, eax
FN_PREFIX(CryptonightR_instruction195):
	add	ebx, eax
	add	ebx, 2147483647
FN_PREFIX(CryptonightR_instruction196):
	sub	ebx, eax
FN_PREFIX(CryptonightR_instruction197):
	ror	ebx, cl
FN_PREFIX(CryptonightR_instruction198):
	rol	ebx, cl
FN_PREFIX(CryptonightR_instruction199):
	xor	ebx, eax
FN_PREFIX(CryptonightR_instruction200):
	imul	esi, eax
FN_PREFIX(CryptonightR_instruction201):
	imul	esi, eax
FN_PREFIX(CryptonightR_instruction202):
	imul	esi, eax
FN_PREFIX(CryptonightR_instruction203):
	add	esi, eax
	add	esi, 2147483647
FN_PREFIX(CryptonightR_instruction204):
	sub	esi, eax
FN_PREFIX(CryptonightR_instruction205):
	ror	esi, cl
FN_PREFIX(CryptonightR_instruction206):
	rol	esi, cl
FN_PREFIX(CryptonightR_instruction207):
	xor	esi, eax
FN_PREFIX(CryptonightR_instruction208):
	imul	edi, eax
FN_PREFIX(CryptonightR_instruction209):
	imul	edi, eax
FN_PREFIX(CryptonightR_instruction210):
	imul	edi, eax
FN_PREFIX(CryptonightR_instruction211):
	add	edi, eax
	add	edi, 2147483647
FN_PREFIX(CryptonightR_instruction212):
	sub	edi, eax
FN_PREFIX(CryptonightR_instruction213):
	ror	edi, cl
FN_PREFIX(CryptonightR_instruction214):
	rol	edi, cl
FN_PREFIX(CryptonightR_instruction215):
	xor	edi, eax
FN_PREFIX(CryptonightR_instruction216):
	imul	ebp, eax
FN_PREFIX(CryptonightR_instruction217):
	imul	ebp, eax
FN_PREFIX(CryptonightR_instruction218):
	imul	ebp, eax
FN_PREFIX(CryptonightR_instruction219):
	add	ebp, eax
	add	ebp, 2147483647
FN_PREFIX(CryptonightR_instruction220):
	sub	ebp, eax
FN_PREFIX(CryptonightR_instruction221):
	ror	ebp, cl
FN_PREFIX(CryptonightR_instruction222):
	rol	ebp, cl
FN_PREFIX(CryptonightR_instruction223):
	xor	ebp, eax
FN_PREFIX(CryptonightR_instruction224):
	imul	ebx, edx
FN_PREFIX(CryptonightR_instruction225):
	imul	ebx, edx
FN_PREFIX(CryptonightR_instruction226):
	imul	ebx, edx
FN_PREFIX(CryptonightR_instruction227):
	add	ebx, edx
	add	ebx, 2147483647
FN_PREFIX(CryptonightR_instruction228):
	sub	ebx, edx
FN_PREFIX(CryptonightR_instruction229):
	ror	ebx, cl
FN_PREFIX(CryptonightR_instruction230):
	rol	ebx, cl
FN_PREFIX(CryptonightR_instruction231):
	xor	ebx, edx
FN_PREFIX(CryptonightR_instruction232):
	imul	esi, edx
FN_PREFIX(CryptonightR_instruction233):
	imul	esi, edx
FN_PREFIX(CryptonightR_instruction234):
	imul	esi, edx
FN_PREFIX(CryptonightR_instruction235):
	add	esi, edx
	add	esi, 2147483647
FN_PREFIX(CryptonightR_instruction236):
	sub	esi, edx
FN_PREFIX(CryptonightR_instruction237):
	ror	esi, cl
FN_PREFIX(CryptonightR_instruction238):
	rol	esi, cl
FN_PREFIX(CryptonightR_instruction239):
	xor	esi, edx
FN_PREFIX(CryptonightR_instruction240):
	imul	edi, edx
FN_PREFIX(CryptonightR_instruction241):
	imul	edi, edx
FN_PREFIX(CryptonightR_instruction242):
	imul	edi, edx
FN_PREFIX(CryptonightR_instruction243):
	add	edi, edx
	add	edi, 2147483647
FN_PREFIX(CryptonightR_instruction244):
	sub	edi, edx
FN_PREFIX(CryptonightR_instruction245):
	ror	edi, cl
FN_PREFIX(CryptonightR_instruction246):
	rol	edi, cl
FN_PREFIX(CryptonightR_instruction247):
	xor	edi, edx
FN_PREFIX(CryptonightR_instruction248):
	imul	ebp, edx
FN_PREFIX(CryptonightR_instruction249):
	imul	ebp, edx
FN_PREFIX(CryptonightR_instruction250):
	imul	ebp, edx
FN_PREFIX(CryptonightR_instruction251):
	add	ebp, edx
	add	ebp, 2147483647
FN_PREFIX(CryptonightR_instruction252):
	sub	ebp, edx
FN_PREFIX(CryptonightR_instruction253):
	ror	ebp, cl
FN_PREFIX(CryptonightR_instruction254):
	rol	ebp, cl
FN_PREFIX(CryptonightR_instruction255):
	xor	ebp, edx
FN_PREFIX(CryptonightR_instruction256):
	imul	ebx, ebx
FN_PREFIX(CryptonightR_instruction_mov0):

FN_PREFIX(CryptonightR_instruction_mov1):

FN_PREFIX(CryptonightR_instruction_mov2):

FN_PREFIX(CryptonightR_instruction_mov3):

FN_PREFIX(CryptonightR_instruction_mov4):

FN_PREFIX(CryptonightR_instruction_mov5):
	mov	ecx, ebx
FN_PREFIX(CryptonightR_instruction_mov6):
	mov	ecx, ebx
FN_PREFIX(CryptonightR_instruction_mov7):

FN_PREFIX(CryptonightR_instruction_mov8):

FN_PREFIX(CryptonightR_instruction_mov9):

FN_PREFIX(CryptonightR_instruction_mov10):

FN_PREFIX(CryptonightR_instruction_mov11):

FN_PREFIX(CryptonightR_instruction_mov12):

FN_PREFIX(CryptonightR_instruction_mov13):
	mov	ecx, ebx
FN_PREFIX(CryptonightR_instruction_mov14):
	mov	ecx, ebx
FN_PREFIX(CryptonightR_instruction_mov15):

FN_PREFIX(CryptonightR_instruction_mov16):

FN_PREFIX(CryptonightR_instruction_mov17):

FN_PREFIX(CryptonightR_instruction_mov18):

FN_PREFIX(CryptonightR_instruction_mov19):

FN_PREFIX(CryptonightR_instruction_mov20):

FN_PREFIX(CryptonightR_instruction_mov21):
	mov	ecx, ebx
FN_PREFIX(CryptonightR_instruction_mov22):
	mov	ecx, ebx
FN_PREFIX(CryptonightR_instruction_mov23):

FN_PREFIX(CryptonightR_instruction_mov24):

FN_PREFIX(CryptonightR_instruction_mov25):

FN_PREFIX(CryptonightR_instruction_mov26):

FN_PREFIX(CryptonightR_instruction_mov27):

FN_PREFIX(CryptonightR_instruction_mov28):

FN_PREFIX(CryptonightR_instruction_mov29):
	mov	ecx, ebx
FN_PREFIX(CryptonightR_instruction_mov30):
	mov	ecx, ebx
FN_PREFIX(CryptonightR_instruction_mov31):

FN_PREFIX(CryptonightR_instruction_mov32):

FN_PREFIX(CryptonightR_instruction_mov33):

FN_PREFIX(CryptonightR_instruction_mov34):

FN_PREFIX(CryptonightR_instruction_mov35):

FN_PREFIX(CryptonightR_instruction_mov36):

FN_PREFIX(CryptonightR_instruction_mov37):
	mov	ecx, esi
FN_PREFIX(CryptonightR_instruction_mov38):
	mov	ecx, esi
FN_PREFIX(CryptonightR_instruction_mov39):

FN_PREFIX(CryptonightR_instruction_mov40):

FN_PREFIX(CryptonightR_instruction_mov41):

FN_PREFIX(CryptonightR_instruction_mov42):

FN_PREFIX(CryptonightR_instruction_mov43):

FN_PREFIX(CryptonightR_instruction_mov44):

FN_PREFIX(CryptonightR_instruction_mov45):
	mov	ecx, esi
FN_PREFIX(CryptonightR_instruction_mov46):
	mov	ecx, esi
FN_PREFIX(CryptonightR_instruction_mov47):

FN_PREFIX(CryptonightR_instruction_mov48):

FN_PREFIX(CryptonightR_instruction_mov49):

FN_PREFIX(CryptonightR_instruction_mov50):

FN_PREFIX(CryptonightR_instruction_mov51):

FN_PREFIX(CryptonightR_instruction_mov52):

FN_PREFIX(CryptonightR_instruction_mov53):
	mov	ecx, esi
FN_PREFIX(CryptonightR_instruction_mov54):
	mov	ecx, esi
FN_PREFIX(CryptonightR_instruction_mov55):

FN_PREFIX(CryptonightR_instruction_mov56):

FN_PREFIX(CryptonightR_instruction_mov57):

FN_PREFIX(CryptonightR_instruction_mov58):

FN_PREFIX(CryptonightR_instruction_mov59):

FN_PREFIX(CryptonightR_instruction_mov60):

FN_PREFIX(CryptonightR_instruction_mov61):
	mov	ecx, esi
FN_PREFIX(CryptonightR_instruction_mov62):
	mov	ecx, esi
FN_PREFIX(CryptonightR_instruction_mov63):

FN_PREFIX(CryptonightR_instruction_mov64):

FN_PREFIX(CryptonightR_instruction_mov65):

FN_PREFIX(CryptonightR_instruction_mov66):

FN_PREFIX(CryptonightR_instruction_mov67):

FN_PREFIX(CryptonightR_instruction_mov68):

FN_PREFIX(CryptonightR_instruction_mov69):
	mov	ecx, edi
FN_PREFIX(CryptonightR_instruction_mov70):
	mov	ecx, edi
FN_PREFIX(CryptonightR_instruction_mov71):

FN_PREFIX(CryptonightR_instruction_mov72):

FN_PREFIX(CryptonightR_instruction_mov73):

FN_PREFIX(CryptonightR_instruction_mov74):

FN_PREFIX(CryptonightR_instruction_mov75):

FN_PREFIX(CryptonightR_instruction_mov76):

FN_PREFIX(CryptonightR_instruction_mov77):
	mov	ecx, edi
FN_PREFIX(CryptonightR_instruction_mov78):
	mov	ecx, edi
FN_PREFIX(CryptonightR_instruction_mov79):

FN_PREFIX(CryptonightR_instruction_mov80):

FN_PREFIX(CryptonightR_instruction_mov81):

FN_PREFIX(CryptonightR_instruction_mov82):

FN_PREFIX(CryptonightR_instruction_mov83):

FN_PREFIX(CryptonightR_instruction_mov84):

FN_PREFIX(CryptonightR_instruction_mov85):
	mov	ecx, edi
FN_PREFIX(CryptonightR_instruction_mov86):
	mov	ecx, edi
FN_PREFIX(CryptonightR_instruction_mov87):

FN_PREFIX(CryptonightR_instruction_mov88):

FN_PREFIX(CryptonightR_instruction_mov89):

FN_PREFIX(CryptonightR_instruction_mov90):

FN_PREFIX(CryptonightR_instruction_mov91):

FN_PREFIX(CryptonightR_instruction_mov92):

FN_PREFIX(CryptonightR_instruction_mov93):
	mov	ecx, edi
FN_PREFIX(CryptonightR_instruction_mov94):
	mov	ecx, edi
FN_PREFIX(CryptonightR_instruction_mov95):

FN_PREFIX(CryptonightR_instruction_mov96):

FN_PREFIX(CryptonightR_instruction_mov97):

FN_PREFIX(CryptonightR_instruction_mov98):

FN_PREFIX(CryptonightR_instruction_mov99):

FN_PREFIX(CryptonightR_instruction_mov100):

FN_PREFIX(CryptonightR_instruction_mov101):
	mov	ecx, ebp
FN_PREFIX(CryptonightR_instruction_mov102):
	mov	ecx, ebp
FN_PREFIX(CryptonightR_instruction_mov103):

FN_PREFIX(CryptonightR_instruction_mov104):

FN_PREFIX(CryptonightR_instruction_mov105):

FN_PREFIX(CryptonightR_instruction_mov106):

FN_PREFIX(CryptonightR_instruction_mov107):

FN_PREFIX(CryptonightR_instruction_mov108):

FN_PREFIX(CryptonightR_instruction_mov109):
	mov	ecx, ebp
FN_PREFIX(CryptonightR_instruction_mov110):
	mov	ecx, ebp
FN_PREFIX(CryptonightR_instruction_mov111):

FN_PREFIX(CryptonightR_instruction_mov112):

FN_PREFIX(CryptonightR_instruction_mov113):

FN_PREFIX(CryptonightR_instruction_mov114):

FN_PREFIX(CryptonightR_instruction_mov115):

FN_PREFIX(CryptonightR_instruction_mov116):

FN_PREFIX(CryptonightR_instruction_mov117):
	mov	ecx, ebp
FN_PREFIX(CryptonightR_instruction_mov118):
	mov	ecx, ebp
FN_PREFIX(CryptonightR_instruction_mov119):

FN_PREFIX(CryptonightR_instruction_mov120):

FN_PREFIX(CryptonightR_instruction_mov121):

FN_PREFIX(CryptonightR_instruction_mov122):

FN_PREFIX(CryptonightR_instruction_mov123):

FN_PREFIX(CryptonightR_instruction_mov124):

FN_PREFIX(CryptonightR_instruction_mov125):
	mov	ecx, ebp
FN_PREFIX(CryptonightR_instruction_mov126):
	mov	ecx, ebp
FN_PREFIX(CryptonightR_instruction_mov127):

FN_PREFIX(CryptonightR_instruction_mov128):

FN_PREFIX(CryptonightR_instruction_mov129):

FN_PREFIX(CryptonightR_instruction_mov130):

FN_PREFIX(CryptonightR_instruction_mov131):

FN_PREFIX(CryptonightR_instruction_mov132):

FN_PREFIX(CryptonightR_instruction_mov133):
	mov	ecx, esp
FN_PREFIX(CryptonightR_instruction_mov134):
	mov	ecx, esp
FN_PREFIX(CryptonightR_instruction_mov135):

FN_PREFIX(CryptonightR_instruction_mov136):

FN_PREFIX(CryptonightR_instruction_mov137):

FN_PREFIX(CryptonightR_instruction_mov138):

FN_PREFIX(CryptonightR_instruction_mov139):

FN_PREFIX(CryptonightR_instruction_mov140):

FN_PREFIX(CryptonightR_instruction_mov141):
	mov	ecx, esp
FN_PREFIX(CryptonightR_instruction_mov142):
	mov	ecx, esp
FN_PREFIX(CryptonightR_instruction_mov143):

FN_PREFIX(CryptonightR_instruction_mov144):

FN_PREFIX(CryptonightR_instruction_mov145):

FN_PREFIX(CryptonightR_instruction_mov146):

FN_PREFIX(CryptonightR_instruction_mov147):

FN_PREFIX(CryptonightR_instruction_mov148):

FN_PREFIX(CryptonightR_instruction_mov149):
	mov	ecx, esp
FN_PREFIX(CryptonightR_instruction_mov150):
	mov	ecx, esp
FN_PREFIX(CryptonightR_instruction_mov151):

FN_PREFIX(CryptonightR_instruction_mov152):

FN_PREFIX(CryptonightR_instruction_mov153):

FN_PREFIX(CryptonightR_instruction_mov154):

FN_PREFIX(CryptonightR_instruction_mov155):

FN_PREFIX(CryptonightR_instruction_mov156):

FN_PREFIX(CryptonightR_instruction_mov157):
	mov	ecx, esp
FN_PREFIX(CryptonightR_instruction_mov158):
	mov	ecx, esp
FN_PREFIX(CryptonightR_instruction_mov159):

FN_PREFIX(CryptonightR_instruction_mov160):

FN_PREFIX(CryptonightR_instruction_mov161):

FN_PREFIX(CryptonightR_instruction_mov162):

FN_PREFIX(CryptonightR_instruction_mov163):

FN_PREFIX(CryptonightR_instruction_mov164):

FN_PREFIX(CryptonightR_instruction_mov165):
	mov	ecx, r15d
FN_PREFIX(CryptonightR_instruction_mov166):
	mov	ecx, r15d
FN_PREFIX(CryptonightR_instruction_mov167):

FN_PREFIX(CryptonightR_instruction_mov168):

FN_PREFIX(CryptonightR_instruction_mov169):

FN_PREFIX(CryptonightR_instruction_mov170):

FN_PREFIX(CryptonightR_instruction_mov171):

FN_PREFIX(CryptonightR_instruction_mov172):

FN_PREFIX(CryptonightR_instruction_mov173):
	mov	ecx, r15d
FN_PREFIX(CryptonightR_instruction_mov174):
	mov	ecx, r15d
FN_PREFIX(CryptonightR_instruction_mov175):

FN_PREFIX(CryptonightR_instruction_mov176):

FN_PREFIX(CryptonightR_instruction_mov177):

FN_PREFIX(CryptonightR_instruction_mov178):

FN_PREFIX(CryptonightR_instruction_mov179):

FN_PREFIX(CryptonightR_instruction_mov180):

FN_PREFIX(CryptonightR_instruction_mov181):
	mov	ecx, r15d
FN_PREFIX(CryptonightR_instruction_mov182):
	mov	ecx, r15d
FN_PREFIX(CryptonightR_instruction_mov183):

FN_PREFIX(CryptonightR_instruction_mov184):

FN_PREFIX(CryptonightR_instruction_mov185):

FN_PREFIX(CryptonightR_instruction_mov186):

FN_PREFIX(CryptonightR_instruction_mov187):

FN_PREFIX(CryptonightR_instruction_mov188):

FN_PREFIX(CryptonightR_instruction_mov189):
	mov	ecx, r15d
FN_PREFIX(CryptonightR_instruction_mov190):
	mov	ecx, r15d
FN_PREFIX(CryptonightR_instruction_mov191):

FN_PREFIX(CryptonightR_instruction_mov192):

FN_PREFIX(CryptonightR_instruction_mov193):

FN_PREFIX(CryptonightR_instruction_mov194):

FN_PREFIX(CryptonightR_instruction_mov195):

FN_PREFIX(CryptonightR_instruction_mov196):

FN_PREFIX(CryptonightR_instruction_mov197):
	mov	ecx, eax
FN_PREFIX(CryptonightR_instruction_mov198):
	mov	ecx, eax
FN_PREFIX(CryptonightR_instruction_mov199):

FN_PREFIX(CryptonightR_instruction_mov200):

FN_PREFIX(CryptonightR_instruction_mov201):

FN_PREFIX(CryptonightR_instruction_mov202):

FN_PREFIX(CryptonightR_instruction_mov203):

FN_PREFIX(CryptonightR_instruction_mov204):

FN_PREFIX(CryptonightR_instruction_mov205):
	mov	ecx, eax
FN_PREFIX(CryptonightR_instruction_mov206):
	mov	ecx, eax
FN_PREFIX(CryptonightR_instruction_mov207):

FN_PREFIX(CryptonightR_instruction_mov208):

FN_PREFIX(CryptonightR_instruction_mov209):

FN_PREFIX(CryptonightR_instruction_mov210):

FN_PREFIX(CryptonightR_instruction_mov211):

FN_PREFIX(CryptonightR_instruction_mov212):

FN_PREFIX(CryptonightR_instruction_mov213):
	mov	ecx, eax
FN_PREFIX(CryptonightR_instruction_mov214):
	mov	ecx, eax
FN_PREFIX(CryptonightR_instruction_mov215):

FN_PREFIX(CryptonightR_instruction_mov216):

FN_PREFIX(CryptonightR_instruction_mov217):

FN_PREFIX(CryptonightR_instruction_mov218):

FN_PREFIX(CryptonightR_instruction_mov219):

FN_PREFIX(CryptonightR_instruction_mov220):

FN_PREFIX(CryptonightR_instruction_mov221):
	mov	ecx, eax
FN_PREFIX(CryptonightR_instruction_mov222):
	mov	ecx, eax
FN_PREFIX(CryptonightR_instruction_mov223):

FN_PREFIX(CryptonightR_instruction_mov224):

FN_PREFIX(CryptonightR_instruction_mov225):

FN_PREFIX(CryptonightR_instruction_mov226):

FN_PREFIX(CryptonightR_instruction_mov227):

FN_PREFIX(CryptonightR_instruction_mov228):

FN_PREFIX(CryptonightR_instruction_mov229):
	mov	ecx, edx
FN_PREFIX(CryptonightR_instruction_mov230):
	mov	ecx, edx
FN_PREFIX(CryptonightR_instruction_mov231):

FN_PREFIX(CryptonightR_instruction_mov232):

FN_PREFIX(CryptonightR_instruction_mov233):

FN_PREFIX(CryptonightR_instruction_mov234):

FN_PREFIX(CryptonightR_instruction_mov235):

FN_PREFIX(CryptonightR_instruction_mov236):

FN_PREFIX(CryptonightR_instruction_mov237):
	mov	ecx, edx
FN_PREFIX(CryptonightR_instruction_mov238):
	mov	ecx, edx
FN_PREFIX(CryptonightR_instruction_mov239):

FN_PREFIX(CryptonightR_instruction_mov240):

FN_PREFIX(CryptonightR_instruction_mov241):

FN_PREFIX(CryptonightR_instruction_mov242):

FN_PREFIX(CryptonightR_instruction_mov243):

FN_PREFIX(CryptonightR_instruction_mov244):

FN_PREFIX(CryptonightR_instruction_mov245):
	mov	ecx, edx
FN_PREFIX(CryptonightR_instruction_mov246):
	mov	ecx, edx
FN_PREFIX(CryptonightR_instruction_mov247):

FN_PREFIX(CryptonightR_instruction_mov248):

FN_PREFIX(CryptonightR_instruction_mov249):

FN_PREFIX(CryptonightR_instruction_mov250):

FN_PREFIX(CryptonightR_instruction_mov251):

FN_PREFIX(CryptonightR_instruction_mov252):

FN_PREFIX(CryptonightR_instruction_mov253):
	mov	ecx, edx
FN_PREFIX(CryptonightR_instruction_mov254):
	mov	ecx, edx
FN_PREFIX(CryptonightR_instruction_mov255):

FN_PREFIX(CryptonightR_instruction_mov256):
