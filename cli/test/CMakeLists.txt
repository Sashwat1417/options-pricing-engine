# Smoke tests: run the `ope` binary and check its output. Hull Ch 15 example.

set(hull --spot 42 --strike 40 --expiry 0.5 --rate 0.1)

add_test(NAME cli.price.call COMMAND ope_cli price --type call ${hull} --vol 0.2)
set_tests_properties(cli.price.call PROPERTIES PASS_REGULAR_EXPRESSION "^4\\.759422")

add_test(NAME cli.price.put COMMAND ope_cli price --type put ${hull} --vol 0.2)
set_tests_properties(cli.price.put PROPERTIES PASS_REGULAR_EXPRESSION "^0\\.808599")

add_test(NAME cli.iv COMMAND ope_cli iv --type call ${hull} --price 4.76)
set_tests_properties(cli.iv PROPERTIES PASS_REGULAR_EXPRESSION "^0\\.200066")

add_test(NAME cli.sweep COMMAND ope_cli sweep ${hull} --vol 0.2 --vary spot --from 30 --to 50 --steps 3)
set_tests_properties(cli.sweep PROPERTIES
    PASS_REGULAR_EXPRESSION "^spot,call,put,call_payoff,put_payoff\n30,.*\n40,.*\n50,")

add_test(NAME cli.rejects_invalid_input COMMAND ope_cli price --type call ${hull} --vol -0.2)
set_tests_properties(cli.rejects_invalid_input PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli.sweep_needs_vol COMMAND ope_cli sweep ${hull} --vary spot --from 30 --to 50)
set_tests_properties(cli.sweep_needs_vol PROPERTIES WILL_FAIL TRUE)
