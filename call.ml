external call_python: string -> int -> int = "call_python"

let pass_integer n = n;;

let test_call = call_python "pass_integer" 10;;

let () =
    Callback.register "pass_integer" pass_integer;
    Callback.register "test_call" test_call;
;;