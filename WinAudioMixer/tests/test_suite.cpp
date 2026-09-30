extern void run_config_test();
extern void run_filter_test();
extern void run_deadzone_test();
extern void run_mapping_test();

int main()
{
    run_config_test();
    run_filter_test();
    run_deadzone_test();
    run_mapping_test();
    return 0;
}
