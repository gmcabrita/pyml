external call_python: string -> string list -> int = "call_python"

exception Pyml_exception of string;;

let pass_integer n = n;;

let test_raise n = raise(Pyml_exception "pyml_test_exception");;

let test_custom_exception () =
    try call_python "test_raise" ["0"] with
        Pyml_exception msg -> 1
;;

let divide_by n = 5 / n;;

let test_exception () =
    try call_python "divide_by" ["0"] with
        Pyml_exception msg -> 1
;;

let test_call () = call_python "pass_integer" ["10"];;

let divide x y = x / y;;

let test_n_args () = call_python "divide" ["10";"2"];;

let () =
    Callback.register "pass_integer" pass_integer;
    Callback.register "test_call" test_call;
    Callback.register "test_raise" test_raise;
    Callback.register "test_exception" test_exception;
    Callback.register "test_custom_exception" test_custom_exception;
    Callback.register "divide_by" divide_by;
    Callback.register "Printexc.to_string" Printexc.to_string;
    Callback.register "divide" divide;
    Callback.register "test_n_args" test_n_args;
    Callback.register_exception "pyml_exception" (Pyml_exception "pyml_test_exception");
;;
