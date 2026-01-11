fn example_interceptor(name: str, args: any[], continue: fn(): any): any =>

  // type checking and casting of args
  if args.length > 0 && typeof(args[0]) is str =>
    print($"arg[0]:str = {arg[0] as str} - substituting with 'apple'")
    arg[0] = "apple" // overwrites first parameter value, panics if type is wrong

  if args.length > 1 && typeof(args[1]) is bool =>
    print($"arg[1]:bool = {arg[1] as bool}") // just prints value, casting if type check matches

  var result:any = continue()

  if result is str => 
    print("Result is " + result + "\n")

  return 200 // fake result overidden, otherwise just return result

fn intercepted_call(s:str, i:int, b:bool):int => 
  print($"function called with s={s}, i={i}, b={b}")
  return 99

fn main => 
  println("Starting interceptor demo")

  Interceptor.register(example_interceptor)
  
  var result:int = intercepted_call("hello world!", 5, true) // intercepted
  println($"result={result}") // result = 200

  Interceptor.clearAll()

  var result:int = intercepted_call("hello world!", 5, true) // not intercepted
  println($"result={result}") // result = 99

