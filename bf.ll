; brainfuck interpreter
; codes not more than 2,000 chars
; code & input split by ':'
; 30,000 cells with u8 (0 ~ 255 wraparound)

declare i32 @getchar()

declare i32 @putchar(i32)

define i32 @main() {
entry:
    %cmd = alloca [2000 x i32]
    %cmd.head = alloca i32*
    %cmd.tail = alloca i32*    
    %cells = alloca [30000 x i32]
    %ptr = alloca i32*
    %tag = alloca [1000 x i32*]
    %tag.top = alloca i32**
    %cnt = alloca i32
    br label %init

init:
    ; cmd.head cmd.tail
    %cmd.begin = getelementptr inbounds [2000 x i32], [2000 x i32]* %cmd, i64 0, i64 0
    %cmd.end = getelementptr inbounds [2000 x i32], [2000 x i32]* %cmd, i64 0, i64 2000
    store i32* %cmd.begin, i32** %cmd.head
    store i32* %cmd.begin, i32** %cmd.tail
    ; ptr
    %ptr.init = getelementptr inbounds [30000 x i32], [30000 x i32]* %cells, i64 0, i64 0
    store i32* %ptr.init, i32** %ptr
    ; top
    %tag.init = getelementptr inbounds [1000 x i32*], [1000 x i32*]* %tag, i64 0, i64 0
    store i32** %tag.init, i32*** %tag.top
    store i32 0, i32* %cnt
    br label %read.get

read.get:
    %char.get = call i32 @getchar()
    %is.read.sep = icmp eq i32 %char.get, 58
    %is.read.eof = icmp eq i32 %char.get, -1
    %is.read.end = or i1 %is.read.sep, %is.read.eof
    %cmd.read.tail.cur = load i32*, i32** %cmd.tail
    %is.cmd.full = icmp eq i32* %cmd.read.tail.cur, %cmd.end
    %is.not.push = or i1 %is.cmd.full, %is.read.end
    br i1 %is.not.push, label %read.end, label %read.push

read.push:
    store i32 %char.get, i32* %cmd.read.tail.cur
    %cmd.tail.nxt = getelementptr inbounds i32, i32* %cmd.read.tail.cur, i32 1
    store i32* %cmd.tail.nxt, i32** %cmd.tail
    br label %read.get

read.end:
    br label %run.entry

run.entry:
    br label %run.cond

run.cond:
    %cmd.run.head.cur = load i32*, i32** %cmd.head
    %cmd.run.tail.cur = load i32*, i32** %cmd.tail
    %is.cmd.empty = icmp eq i32* %cmd.run.head.cur, %cmd.run.tail.cur
    br i1 %is.cmd.empty, label %run.end, label %run.then

run.then:
    %char.cur = load i32, i32* %cmd.run.head.cur
    %cmd.head.nxt = getelementptr inbounds i32, i32* %cmd.run.head.cur, i32 1
    store i32* %cmd.head.nxt, i32** %cmd.head
    switch i32 %char.cur, label %run.entry [
        i32 62, label %run.next
        i32 60, label %run.prev
        i32 43, label %run.inc
        i32 45, label %run.dec
        i32 46, label %run.put
        i32 44, label %run.get
        i32 91, label %run.left
        i32 93, label %run.right
    ]

run.next:
    %ptr.next.cur = load i32*, i32** %ptr
    %ptr.next.nxt = getelementptr inbounds i32, i32* %ptr.next.cur, i32 1
    store i32* %ptr.next.nxt, i32** %ptr
    br label %run.entry

run.prev:
    %ptr.prev.cur = load i32*, i32** %ptr
    %ptr.prev.pre = getelementptr inbounds i32, i32* %ptr.prev.cur, i32 -1
    store i32* %ptr.prev.pre, i32** %ptr
    br label %run.entry

run.inc:
    %ptr.inc = load i32*, i32** %ptr
    %cell.inc.cur = load i32, i32* %ptr.inc
    %is.inc.normal = icmp ne i32 %cell.inc.cur, 255
    %cell.inc.inc = add i32 %cell.inc.cur, 1
    %cell.inc.nxt = select i1 %is.inc.normal, i32 %cell.inc.inc, i32 0
    store i32 %cell.inc.nxt, i32* %ptr.inc
    br label %run.entry

run.dec:
    %ptr.dec = load i32*, i32** %ptr
    %cell.dec.cur = load i32, i32* %ptr.dec
    %is.dec.normal = icmp ne i32 %cell.dec.cur, 0
    %cell.dec.dec = sub i32 %cell.dec.cur, 1
    %cell.dec.nxt = select i1 %is.dec.normal, i32 %cell.dec.dec, i32 255
    store i32 %cell.dec.nxt, i32* %ptr.dec
    br label %run.entry

run.put:
    %ptr.put = load i32*, i32** %ptr
    %cell.put.cur = load i32, i32* %ptr.put
    call i32 @putchar(i32 %cell.put.cur)
    br label %run.entry

run.get:
    %ptr.get = load i32*, i32** %ptr
    %cell.get.nxt = call i32 @getchar()
    store i32 %cell.get.nxt, i32* %ptr.get
    br label %run.entry

run.left:
    %ptr.left = load i32*, i32** %ptr
    %cell.left = load i32, i32* %ptr.left
    %is.left.skip = icmp eq i32 %cell.left, 0
    br i1 %is.left.skip, label %run.left.skip.entry, label %run.left.loop

run.left.loop:
    ; push left tag
    %top.left.loop.cur = load i32**, i32*** %tag.top
    %cmd.left.loop.head.cur = load i32*, i32** %cmd.head
    store i32* %cmd.left.loop.head.cur, i32** %top.left.loop.cur
    %top.left.loop.nxt = getelementptr inbounds i32*, i32** %top.left.loop.cur, i32 1
    store i32** %top.left.loop.nxt, i32*** %tag.top
    br label %run.entry

run.left.skip.entry:
    store i32 1, i32* %cnt
    br label %run.left.skip.body

run.left.skip.body:
    ; jump to match right
    %cmd.left.skip.cur = load i32*, i32** %cmd.head
    %char.skip = load i32, i32* %cmd.left.skip.cur
    %cmd.left.skip.nxt = getelementptr inbounds i32, i32* %cmd.left.skip.cur, i32 1
    store i32* %cmd.left.skip.nxt, i32** %cmd.head
    switch i32 %char.skip, label %run.left.skip.normal [
        i32 91, label %run.left.skip.left
        i32 93, label %run.left.skip.right
    ]

run.left.skip.normal:
    br label %run.left.skip.cond

run.left.skip.left:
    %cnt.left.cur = load i32, i32* %cnt
    %cnt.left.nxt = add i32 %cnt.left.cur, 1
    store i32 %cnt.left.nxt, i32* %cnt
    br label %run.left.skip.cond

run.left.skip.right:
    %cnt.right.cur = load i32, i32* %cnt
    %cnt.right.nxt = sub i32 %cnt.right.cur, 1
    store i32 %cnt.right.nxt, i32* %cnt
    br label %run.left.skip.cond

run.left.skip.cond:
    %cnt.cond = load i32, i32* %cnt
    %is.cnt.zero = icmp eq i32 %cnt.cond, 0
    %is.right = icmp eq i32 %char.skip, 93
    %is.continue = and i1 %is.right, %is.cnt.zero
    br i1 %is.continue, label %run.left.skip.end, label %run.left.skip.body

run.left.skip.end:
    br label %run.entry

run.right:
    %ptr.right = load i32*, i32** %ptr
    %cell.right = load i32, i32* %ptr.right
    %is.right.loop = icmp ne i32 %cell.right, 0
    br i1 %is.right.loop, label %run.right.loop, label %run.right.noloop

run.right.loop:
    ; jump to tag
    %top.right.loop.cur = load i32**, i32*** %tag.top
    %top.right.loop.pre = getelementptr inbounds i32*, i32** %top.right.loop.cur, i32 -1
    %cmd.right.loop.head = load i32*, i32** %top.right.loop.pre
    store i32* %cmd.right.loop.head, i32** %cmd.head
    br label %run.entry

run.right.noloop:
    ; pop left tag
    %top.right.noloop.cur = load i32**, i32*** %tag.top
    %top.right.noloop.pre = getelementptr inbounds i32*, i32** %top.right.noloop.cur, i32 -1
    store i32** %top.right.noloop.pre, i32*** %tag.top
    br label %run.entry

run.end:
    br label %ret

ret:
    ret i32 0
}
