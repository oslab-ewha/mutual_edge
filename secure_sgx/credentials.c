#include <aws/auth/auth.h>
#include <aws/auth/credentials.h>
#include <aws/common/common.h>
#include <aws/io/logging.h>
#include <aws/io/event_loop.h>
#include <stdio.h>

#include <aws/common/mutex.h>
#include <aws/common/condition_variable.h>
#include <aws/io/host_resolver.h>
#include <aws/io/channel_bootstrap.h>
#include <aws/io/socket.h>

typedef struct {
	int	error_code;
	struct aws_credentials	*credentials;
	struct aws_mutex	mutex;
	struct aws_condition_variable	cond;
} get_cred_ctx_t;

static void
credentials_ready(struct aws_credentials *credentials, int error_code, void *arg)
{
	get_cred_ctx_t	*pctx = (get_cred_ctx_t *)arg;

	pctx->error_code = error_code;
	pctx->credentials = credentials;
	aws_mutex_lock(&pctx->mutex);
	aws_condition_variable_notify_one(&pctx->cond);
	aws_mutex_unlock(&pctx->mutex);
}

struct aws_credentials	*
get_aws_credentials(void)
{
	struct aws_allocator	*allocator = aws_default_allocator();
	struct aws_event_loop_group	*elg = aws_event_loop_group_new_default(allocator, 8, NULL);
	struct aws_host_resolver	*resolver;
	struct aws_host_resolver_default_options	resolver_options = {
		.max_entries = 8, .el_group = elg
	};
	get_cred_ctx_t	ctx;
	
	resolver = aws_host_resolver_new_default(allocator, &resolver_options);

	struct aws_client_bootstrap_options bootstrap_options = {
		.event_loop_group = elg,
		.host_resolver = resolver,
	};
	struct aws_client_bootstrap	*bootstrap;
	
	bootstrap = aws_client_bootstrap_new(allocator, &bootstrap_options);

	struct aws_credentials_provider *provider = NULL;
	struct aws_credentials_provider_chain_default_options provider_options = {
		.bootstrap = bootstrap,
	};

	provider = aws_credentials_provider_new_chain_default(allocator, &provider_options);

	aws_mutex_init(&ctx.mutex);
	aws_condition_variable_init(&ctx.cond);
	ctx.credentials = NULL;

	aws_credentials_provider_get_credentials(provider, credentials_ready, &ctx);
	if (ctx.credentials == NULL) {
		aws_mutex_lock(&ctx.mutex);
		aws_condition_variable_wait(&ctx.cond, &ctx.mutex);
		aws_mutex_unlock(&ctx.mutex);
	}

	if (ctx.error_code != 0) {
		fprintf(stderr, "Failed to get credentials: %d.\n", ctx.error_code);
		exit(1);
	}

	aws_credentials_provider_release(provider);
	aws_client_bootstrap_release(bootstrap);
	aws_host_resolver_release(resolver);
	aws_event_loop_group_release(elg);
	aws_mutex_clean_up(&ctx.mutex);
	aws_condition_variable_clean_up(&ctx.cond);

	return ctx.credentials;
}
