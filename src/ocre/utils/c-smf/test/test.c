#include <stdio.h>
#include "../smf/smf.h"

// Define state entry, exit, and run actions
void state1_entry(void *ctx)
{
	(void)ctx; /* -Wunused-parameter */
	printf("Entering State 1\n");
}

void state1_exit(void *ctx)
{
	(void)ctx; /* -Wunused-parameter */
	printf("Exiting State 1\n");
}

void state1_run(void *ctx)
{
	(void)ctx; /* -Wunused-parameter */
	printf("Running State 1\n");
}

void state2_entry(void *ctx)
{
	(void)ctx; /* -Wunused-parameter */
	printf("Entering State 2\n");
}

void state2_exit(void *ctx)
{
	(void)ctx; /* -Wunused-parameter */
	printf("Exiting State 2\n");
}

void state2_run(void *ctx)
{
	(void)ctx; /* -Wunused-parameter */
	printf("Running State 2\n");
}

// Define states
struct smf_state state1 = {
	.entry = state1_entry,
	.run = state1_run,
	.exit = state1_exit,
#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	.parent = NULL,
#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */
};

struct smf_state state2 = {
	.entry = state2_entry,
	.run = state2_run,
	.exit = state2_exit,
#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	.parent = NULL,
#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */
};

int main()
{
	struct smf_ctx ctx;

	// Initialize the state machine with the initial state
	smf_set_initial(&ctx, &state1);

	// Run the initial state
	printf("Starting State Machine\n");
	smf_run_state(&ctx);

	// Transition to a new state
	printf("Transitioning to State 2\n");
	smf_set_state(&ctx, &state2);

	// Run the new state
	smf_run_state(&ctx);

	printf("State Machine Test Complete\n");
	return 0;
}