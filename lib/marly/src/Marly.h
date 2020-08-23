/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Atom.h"
#include "Containers.h"
#include "Executable.h"
#include "GeneratedValues.h"
#include "MString.h"
#include "Scanner.h"
#include "ScriptingLanguage.h"
#include "SharedPtr.h"
#include "MarlyValue.h"

/*
Marly:
    In the descriptions below TOS is to the right. So "X Y Z" is Z on TOS.

Literals:
    bool, int (32 bit), float (32 bit), String, Char, List, Map
    
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
                Pushes a Map (AA)
                
    @<id>       X ->
                Store X at <id>.
                
    $<id>       -> X
                Push value at <id>.
    
    .<id>       X -> Y
                Push property <id> of X
                
    :<id>       X Y ->
                Store X in property <id> of Map Y
                
    <id>        .. -> ..
                Execute named function.
    
    new         X Y -> Z
                Use values in list X as params to Map Y and create a new instance of Y

    exec        [..] -> ..
                execute list on TOS

    pack        .. n -> [..]
                Makes n stack items into a list.

    unpack      [A0..An] -> A0 .. An
                Places each element from the list onto the stack.

    dup         X -> X X
                Pushes an extra copy of X onto stack.

    swap        X Y -> Y X
                Interchanges X and Y on top of the stack.
                
    pick        A0..Ai..An i -> A0..An Ai
                Remove the nth item on the stack and push it
                
    tuck        A0..A(i-1) Ai..An X i -> A0..A(i-1) X ai..An
                Insert X n locations down the stack

    pop         X ->
                Removes X from top of the stack.

    join        [ X ] [ Y ] -> [X Y]
                Combine 2 lists into one. 
                
    cat         X Y -> "XY"
                Concatenate X and Y. Can be any types which are converted to strings 
    
    remove      [ A0..Ai..An ] i -> [ A0..An ] Ai
                Remove value at index i and push it.

    insert      [ A0..A(i-1) Ai..An ] X i -> [ A0..A(i-1) X ai..An ]
                Insert X before A(i) in the list

    at          [..] i -> X
                Push the ith element in the List.
    
    atput       [..] X i ->
                Replace the ith element of the list with X.
                
    bor         X Y -> Z
                Z is the bitwise or of ints X and Y.

    bxor        X Y -> Z
                Z is the bitwise exclusive or of ints X and Y.

    band        X Y -> Z
                Z is the bitwise and of ints X and Y.

    bnot        X -> Y
                Y is the inverse of int X.
                
    or          X Y -> B
                B is true if either X or Y are true, false otherwise.
    
    and         X Y -> B
                B is true if both X and Y are true, false otherwise.
                
    not         X -> B
                B is true if X is false, false otherwise.
                
    neg         X -> Y
                Negate (2 complement) X
    
    lt          X Y -> B
                B = X < Y

    le          X Y -> B
                B = X <= Y

    eq          X Y -> B
                B = X == Y

    ne          X Y -> B
                B = X != Y

    ge          X Y -> B
                B = X >= Y

    gt          X Y -> B
                B = X > Y

    +           X Y -> Z
                Z = X + Y. Numbers can be int or float.

    -           X Y -> Z
                Z = X - Y. Numbers can be int or float.

    *           X Y -> Z
                Z = X times Y. Numbers can be int or float.

    /           X Y -> Z
                Z = X divided by Y. Numbers can be int or float.

    %           X Y -> Z
                Z = X modulo Y. Numbers can be int or float.

    inc         M -> N
                Increment M by 1.

    dec         M -> N
                Decrement M by 1.
                
    if          B [T] ->
                If B is true execute T othereise skip.
    
    ifte        B [T] [F] ->
                If B is true execute T otherwise execute F.
                
    while       B X ->
                Execute B. If it is true execute X and repeat.
                
    for         S B I X ->
                Execute B. Pop the result from the stack. If result is true execute X then 
                I. Repeat until not true. S is the current iteration value and remains on 
                TOS. It can be modified by any of the lists, but must be the only element 
                left on the stack at the end of execution.

    fold        A V0 [P] -> V
                Starting with value V0, push each member of A and execute P to produce value V.
                
    map         A [P] -> B
                Executes P on each member of aggregate A, collects results in aggregate B.
                
    filter      A [B] -> A1
                Execute B on each element of A. If true that element is added to list A1

    import      "S" -> O
                import package S, pushing O which contains elements of S
*/

namespace marly {

class Marly;

class MarlyScriptingLanguage : public m8r::ScriptingLanguage
{
public:
    virtual const char* suffix() const override { return "marly"; }
    virtual m8r::SharedPtr<m8r::Executable> create() const override;
};

class Marly : public m8r::Executable {
public:
    using Verb = std::function<void()>;
    
    Marly();
    
    virtual bool load(const m8r::Stream&) override;
    virtual m8r::CallReturnValue execute() override;
    virtual const char* runtimeErrorString() const override { return _errorString.c_str(); }
    virtual const m8r::ParseErrorList* parseErrors() const override { return &_parseErrors; }

    const char* stringFromAtom(m8r::Atom atom) const { return _atomTable.stringFromAtom(atom); }
    
    void fireEvent(const Value&) { }

private:
    enum class State { Function, Body, ForTest, ForBody, ForIter, WhileTest, WhileBody, LoopBody };

    bool initExec(const Value& list, State = State::Function);
    void startExec();
    
    bool addParseError(const char* desc)
    {
        _parseErrors.emplace_back(desc, _scanner.lineno());
        return _parseErrors.size() > MaxErrors;
    }
    
    m8r::Scanner _scanner;

    ValueMap _vars;
    m8r::Stack<Value> _stack;
    m8r::Stack<Value> _codeStack;
    m8r::AtomTable _atomTable;
    m8r::Map<m8r::Atom, Verb> _verbs;
    
    static constexpr uint16_t MaxErrors = 32;
    State _currentState = State::Function;
    int32_t _currentIndex = 0;
    m8r::SharedPtr<List> _currentCode;
    
    m8r::String _errorString;
    m8r::ParseErrorList _parseErrors;
};    

}
