declare i32 @printf(i8*, ...)
declare i32 @atoi(i8*)

@.str.int = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.str.double = private unnamed_addr constant [7 x i8] c"%.16e\0A\00"
@.str.true = private unnamed_addr constant [6 x i8] c"true\0A\00"
@.str.false = private unnamed_addr constant [7 x i8] c"false\0A\00"

define void @main(i8** %args) {
  %1 = alloca i32
  %2 = alloca i32
  %3 = alloca double
  %4 = load i32, i32* %2
  %5 = getelementptr [4 x i8], [4 x i8]* @.str.int, i32 0, i32 0
  %6 = call i32 (i8*, ...) @printf(i8* %5, i32 %4)
  %7 = load double, double* %3
  %8 = fadd double 0.0, 2.0
  %9 = fadd double %7, %8
  %10 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %11 = call i32 (i8*, ...) @printf(i8* %10, double %9)
  ret void
}

