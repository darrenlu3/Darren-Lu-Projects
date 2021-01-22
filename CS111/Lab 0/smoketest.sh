#!/usr/local/bin/bash
#NAME: Darren Lu                                          
#EMAIL: darrenlu3@ucla.edu                                  
#ID: 205394473 

#smoke test script for Project 0

if test -f "a.txt"; then
    rm a.txt
fi
if test -f "b.txt"; then
    rm b.txt
fi
if test -f "c.txt"; then
    rm c.txt
fi

let errors=0

#testing normal function of program
test1=`diff <(echo abc | ./lab0) <(echo abc) | wc -l`
#echo $test1
echo abc | ./lab0 >/dev/null
if [ $? != 0 ] ; then
    let errors+=1
    echo "here7"
fi
if [ $test1 != 0 ]; then
    let errors+=1
    echo "here8"
fi

#testing --input function of program                      
./lab0 --input= >/dev/null 2>/dev/null
#echo $?                                                 
if [ $? != 1 ]; then
    let errors+=1
    echo "here9"
fi

./lab0 --input=a.txt >/dev/null 2>/dev/null
if [ $? != 2 ]; then
    echo $?
    let errors+=1
    echo "herea"
fi

echo def > a.txt
test2=`diff <(./lab0 --input=a.txt) <(echo def) | wc -l`
#echo $test2
./lab0 --input=a.txt >/dev/null
if [ $? != 0 ]; then
    let errors+=1
    echo "hereb"
fi
if [ $test2 != 0 ]; then
    let errors+=1
    echo "herec"
fi

#testing --output function
./lab0 --output= >/dev/null 2>/dev/null
if [ $? != 1 ]; then
    let errors+=1
    echo "here"
fi

chmod u-w a.txt
./lab0 --output=a.txt 2>/dev/null
if [ $? != 3 ]; then
    let errors+=1
    echo "here"
fi
chmod u+w a.txt

echo ghi | ./lab0 --output=b.txt
if [ $? != 0 ]; then
    let errors+=1
    echo "here6"
fi
chmod ug+rw b.txt
test3=`diff b.txt <(echo ghi) | wc -l`
#echo $test3
if [ $test3 != 0 ]; then
    let errors+=1
    echo "here5"
fi

#testing --input + --output
./lab0 --input=a.txt --output=b.txt >/dev/null
if [ $? != 0 ]; then
    let errors+=1
    echo "here4"
fi
test4=`diff a.txt b.txt | wc -l`
if [ $test4 != 0 ]; then
    let errors+=1
    echo "here3"
fi

#testing --segfault
./lab0 --segfault >/dev/null 2>/dev/null
if [ $? != 139 ]; then
    let errors+=1
    echo "here2"
fi

./lab0 --segfault --input=a.txt --output=c.txt >/dev/null
chmod u+rw c.txt
if [ 'diff a.txt c.txt | wc -l' == 0 ]; then
    let errors+=1
    echo "here1"
fi

#testing --catch
./lab0 --segfault --catch >/dev/null 2>/dev/null
if [ $? != 4 ]; then
    let errors+=1
    echo "here"
fi

./lab0 --segfault --catch --input=a.txt --output=c.txt >/dev/null 2>/dev/null
if [ 'diff a.txt c.txt | wc -l' == 0 ]; then
    let errors+=1
    echo "here"
fi

#testing for unrecognized arguments
./lab0 --foo --bar >/dev/null 2>/dev/null
if [ $? != 1 ]; then
    let errors+=1
    echo "here"
fi

#echo $errors
if [ $errors == 0 ]; then
    echo "Success, all tests passed"
else
    echo "Failed, $errors failures"
fi
