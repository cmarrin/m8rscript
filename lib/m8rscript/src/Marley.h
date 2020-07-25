/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

/*
Marley:
    In the descriptions below TOS is to the right. So "X Y Z" is Z on TOS.

Literals:
    bool, int (32 bit), float (32 bit), String, Char, List, Object
    
Operators:
    false:      -> false
                Pushes the bool value false.
                
    true:       -> true
                Pushes the bool value true.
    
    <int>:      -> <int>
                Pushes <int>.
                
    <float>:    -> <float>
                Pushes <float>.

    "<chars>"   -> <string>
                Pushes <string>.
                
    '<char>'    -> <char>
                Pushes <char>.

    [ A0..An ]  -> [..]
                Pushes list.
                
    { <id> : <value> ... } -> { .. }
                Pushes an Object (AA)
                
    @<id>       X ->
                Store X at <id>.
                
    $<id>       -> X
                Push value at <id>.
    
    .<id>       X -> Y
                Deref property <id> of X and push the result
                
    <id>        .. -> ..
                Execute named function.
    
    new         X Y -> Z
                Use values in list X as params to Object Y and create a new instance of Y

    x           [..] -> ..
                execute list on TOS

    pack:       .. n -> [..]
                Makes n stack items into a list.

    unpack:     [A0..An] -> A0 .. An
                Places each element from the list onto the stack.

    dup:        X -> X X
                Pushes an extra copy of X onto stack.

    swap:       X Y -> Y X
                Interchanges X and Y on top of the stack.
                
    spick:      A0..Ai..An i -> A0..An Ai
                Remove the nth item on the stack and push it
                
    stuck:      A0..A(i-1) Ai..An X i -> A0..A(i-1) X ai..An
                Insert X n locations down the stack

    pop:        X ->
                Removes X from top of the stack.

    join:       [ X ] [ Y ] -> [X Y]
                Combine 2 lists into one. 
                
    cat:        "X" "Y" -> "XY"
                Concatenate 2 strings
    
    lpick:      [ A0..Ai..An ] i -> [ A0..An ] Ai
                Remove value at index i and push it.

    ltuck:      [ A0..A(i-1) Ai..An ] X i -> [ A0..A(i-1) X ai..An ]
                Insert X before A(i) in the list

    size:       X -> N
                N is the size of X which can be a list or a string

    bor:        X Y -> Z
                Z is the bitwise or of ints X and Y.

    bxor:       X Y -> Z
                Z is the bitwise exclusive or of ints X and Y.

    band:       X Y -> Z
                Z is the bitwise and of ints X and Y.

    bneg:       X -> Y
                Y is the inverse of int X.
                
    or:         X Y -> B
                B is true if either X or Y are true, false otherwise.
    
    and:        X Y -> B
                B is true if both X and Y are true, false otherwise.
                
    not:        X -> B
                B is true if X is false, false otherwise.
                
    +:          X Y -> Z
                Z = X + Y. Numbers can be int or float.

    -:          X Y -> Z
                Z = X - Y. Numbers can be int or float.

    *:          X Y -> Z
                Z = X times Y. Numbers can be int or float.

    /:          X Y -> Z
                Z = X divided by Y. Numbers can be int or float.

    %:          X Y -> Z
                Z = X modulo Y. Numbers can be int or float.

    inc:        M -> N
                Increment M by 1.

    dec:        M -> N
                Decrement M by 1.
                
    if:         B [T] ->
                If B is true execute T othereise skip.
    
    ifte:       B [T] [F] ->
                If B is true execute T otherwise execute F.
                
    while:      B X ->
                Execute B. If it is true execute X and repeat.
                
    for:        S B I X ->
                Execute B. If true execute X then I. Repeat

    fold:       A V0 [P] -> V
                Starting with value V0, push each member of A and execute P to produce value V.
                
    map:        A [P] -> B
                Executes P on each member of aggregate A, collects results in aggregate B.
                
    filter:     A [B] -> A1
                Execute B on each element of A. If true that element is added to list A1

    import      "S" -> O
                import package S, pushing O which contains elements of S
*/
