declare i32 @printf(i8*, ...)
declare i32 @atoi(i8*)

declare double @atof(i8*)

@.str.int = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.str.double = private unnamed_addr constant [7 x i8] c"%.16e\0A\00"
@.str.true = private unnamed_addr constant [6 x i8] c"true\0A\00"
@.str.false = private unnamed_addr constant [7 x i8] c"false\0A\00"
@.str.str = private unnamed_addr constant [4 x i8] c"%s\0A\00"

@.str.0 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.1 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.2 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.3 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.4 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.5 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.6 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.7 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.8 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.9 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.10 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.11 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.12 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.13 = private unnamed_addr constant [2 x i8] c"\0A\00"

@global_var = global i32 0
define i32 @main(i32 %argc, i8** %argv) {
  %1 = alloca i32
  %2 = alloca i1
  %3 = alloca double
  %4 = add i32 0, 1
  store i32 %4, i32* %1
  %5 = load i32, i32* %1
  %6 = getelementptr [4 x i8], [4 x i8]* @.str.int, i32 0, i32 0
  %7 = call i32 (i8*, ...) @printf(i8* %6, i32 %5)
  %8 = getelementptr [2 x i8], [2 x i8]* @.str.0, i32 0, i32 0
  %9 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %10 = call i32 (i8*, ...) @printf(i8* %9, i8* %8)
  %11 = add i32 0, 1
  store i32 %11, i32* %1
  %12 = load i32, i32* %1
  %13 = getelementptr [4 x i8], [4 x i8]* @.str.int, i32 0, i32 0
  %14 = call i32 (i8*, ...) @printf(i8* %13, i32 %12)
  %15 = getelementptr [2 x i8], [2 x i8]* @.str.1, i32 0, i32 0
  %16 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %17 = call i32 (i8*, ...) @printf(i8* %16, i8* %15)
  %18 = add i32 0, 1
  %19 = sub i32 0, %18
  store i32 %19, i32* %1
  %20 = load i32, i32* %1
  %21 = getelementptr [4 x i8], [4 x i8]* @.str.int, i32 0, i32 0
  %22 = call i32 (i8*, ...) @printf(i8* %21, i32 %20)
  %23 = getelementptr [2 x i8], [2 x i8]* @.str.2, i32 0, i32 0
  %24 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %25 = call i32 (i8*, ...) @printf(i8* %24, i8* %23)
  %26 = add i1 0, 1
  store i1 %26, i1* %2
  %27 = load i1, i1* %2
  %28 = getelementptr [6 x i8], [6 x i8]* @.str.true, i32 0, i32 0
  %29 = getelementptr [7 x i8], [7 x i8]* @.str.false, i32 0, i32 0
  %30 = select i1 %27, i8* %28, i8* %29
  %31 = call i32 (i8*, ...) @printf(i8* %30)
  %32 = getelementptr [2 x i8], [2 x i8]* @.str.3, i32 0, i32 0
  %33 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %34 = call i32 (i8*, ...) @printf(i8* %33, i8* %32)
  %35 = add i1 0, 0
  store i1 %35, i1* %2
  %36 = load i1, i1* %2
  %37 = getelementptr [6 x i8], [6 x i8]* @.str.true, i32 0, i32 0
  %38 = getelementptr [7 x i8], [7 x i8]* @.str.false, i32 0, i32 0
  %39 = select i1 %36, i8* %37, i8* %38
  %40 = call i32 (i8*, ...) @printf(i8* %39)
  %41 = getelementptr [2 x i8], [2 x i8]* @.str.4, i32 0, i32 0
  %42 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %43 = call i32 (i8*, ...) @printf(i8* %42, i8* %41)
  %44 = fadd double 0.0, 2.0
  store double %44, double* %3
  %45 = load double, double* %3
  %46 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %47 = call i32 (i8*, ...) @printf(i8* %46, double %45)
  %48 = getelementptr [2 x i8], [2 x i8]* @.str.5, i32 0, i32 0
  %49 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %50 = call i32 (i8*, ...) @printf(i8* %49, i8* %48)
  %51 = fadd double 0.0, 2.2
  store double %51, double* %3
  %52 = load double, double* %3
  %53 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %54 = call i32 (i8*, ...) @printf(i8* %53, double %52)
  %55 = getelementptr [2 x i8], [2 x i8]* @.str.6, i32 0, i32 0
  %56 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %57 = call i32 (i8*, ...) @printf(i8* %56, i8* %55)
  %58 = add i32 0, 2
  store double %58, double* %3
  %59 = load double, double* %3
  %60 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %61 = call i32 (i8*, ...) @printf(i8* %60, double %59)
  %62 = getelementptr [2 x i8], [2 x i8]* @.str.7, i32 0, i32 0
  %63 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %64 = call i32 (i8*, ...) @printf(i8* %63, i8* %62)
  %65 = add i32 0, 1
  store i32 %65, i32* %1
  %66 = load i32, i32* %1
  store double %66, double* %3
  %67 = load double, double* %3
  %68 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %69 = call i32 (i8*, ...) @printf(i8* %68, double %67)
  %70 = getelementptr [2 x i8], [2 x i8]* @.str.8, i32 0, i32 0
  %71 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %72 = call i32 (i8*, ...) @printf(i8* %71, i8* %70)
  %73 = fadd double 0.0, 2.2
  store double %73, double* %3
  %74 = load double, double* %3
  %75 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %76 = call i32 (i8*, ...) @printf(i8* %75, double %74)
  %77 = getelementptr [2 x i8], [2 x i8]* @.str.9, i32 0, i32 0
  %78 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %79 = call i32 (i8*, ...) @printf(i8* %78, i8* %77)
  %80 = fadd double 0.0, 2.2
  %81 = fsub double 0.0, %80
  store double %81, double* %3
  %82 = load double, double* %3
  %83 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %84 = call i32 (i8*, ...) @printf(i8* %83, double %82)
  %85 = getelementptr [2 x i8], [2 x i8]* @.str.10, i32 0, i32 0
  %86 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %87 = call i32 (i8*, ...) @printf(i8* %86, i8* %85)
  %88 = load i32, i32* %1
  store double %88, double* %3
  %89 = load double, double* %3
  %90 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %91 = call i32 (i8*, ...) @printf(i8* %90, double %89)
  %92 = getelementptr [2 x i8], [2 x i8]* @.str.11, i32 0, i32 0
  %93 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %94 = call i32 (i8*, ...) @printf(i8* %93, i8* %92)
  %95 = load i32, i32* %1
  %96 = sub i32 0, %95
  store double %96, double* %3
  %97 = load double, double* %3
  %98 = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0
  %99 = call i32 (i8*, ...) @printf(i8* %98, double %97)
  %100 = getelementptr [2 x i8], [2 x i8]* @.str.12, i32 0, i32 0
  %101 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %102 = call i32 (i8*, ...) @printf(i8* %101, i8* %100)
  %103 = add i32 0, 10
  store i32 %103, i32* @global_var
  %104 = load i32, i32* @global_var
  %105 = getelementptr [4 x i8], [4 x i8]* @.str.int, i32 0, i32 0
  %106 = call i32 (i8*, ...) @printf(i8* %105, i32 %104)
  %107 = getelementptr [2 x i8], [2 x i8]* @.str.13, i32 0, i32 0
  %108 = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0
  %109 = call i32 (i8*, ...) @printf(i8* %108, i8* %107)
  ret i32 0
}

