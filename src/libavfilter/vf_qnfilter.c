#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavutil/avassert.h"
#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"


typedef struct QNFilterContext {
	const AVClass* class;

	const char* enh_type;

} QNFilterContext;

typedef struct ThreadData {
	AVFrame* in, * out;

} ThreadData;


// core function
static int do_filter(AVFilterContext* ctx, void* arg, int jobnr, int nb_jobs)
{
	av_log(NULL, AV_LOG_WARNING, "### Leon's QN filter_frame: core function \n");

	QNFilterContext* privCtx = ctx->priv;
	ThreadData* td = arg;
	AVFrame* dst = td->out;
	AVFrame* src = td->in;

	//frame_copy_video(dst, src);
	return 0;
}

static int filter_frame(AVFilterLink* link, AVFrame* in)
{
	av_log(NULL, AV_LOG_WARNING, "### Leon's QN filter_frame, link %x, frame %x \n", link, in);
	AVFilterContext* avctx = link->dst;
	AVFilterLink* outlink = avctx->outputs[0];
	AVFrame* out;

	//allocate a new buffer, data is null
	out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
	if (!out) {
		av_frame_free(&in);
		return AVERROR(ENOMEM);
	}

	// copy frame prop
	av_frame_copy_props(out, in);
	out->width = outlink->w;
	out->height = outlink->h;

	ThreadData td;
	td.in = in;
	td.out = out;
	int res;
	if (res = avctx->internal->execute(avctx, do_filter, &td, NULL, FFMIN(outlink->h, avctx->graph->nb_threads))) {
		return res;
	}

	av_frame_free(&in);

	return ff_filter_frame(outlink, out);
}

static av_cold int config_output(AVFilterLink *outlink)
{
	AVFilterContext* ctx = outlink->src;
	QNFilterContext* privCtx = ctx->priv;

	//you can modify output width/height here
	outlink->w = ctx->inputs[0]->w;
	outlink->h = ctx->inputs[0]->h;
	av_log(NULL, AV_LOG_DEBUG, "configure output, w h = (%d %d), format %d \n", outlink->w, outlink->h, outlink->format);

	return 0;
}

static av_cold int init(AVFilterContext* ctx)
{
	av_log(NULL, AV_LOG_DEBUG, "init \n");
	QNFilterContext* privCtx = ctx->priv;
	//init something here if you want
	return 0;
}

static av_cold void uninit(AVFilterContext* ctx)
{
	av_log(NULL, AV_LOG_DEBUG, "uninit \n");
	QNFilterContext* privCtx = ctx->priv;
	//uninit something here if you want
}

//currently we just support the most common YUV420, can add more if needed
static int query_formats(AVFilterContext* ctx)
{
	static const enum AVPixelFormat pix_fmts[] = {
		AV_PIX_FMT_YUV420P,
		AV_PIX_FMT_NONE
	};
	AVFilterFormats* fmts_list = ff_make_format_list(pix_fmts);
	if (!fmts_list)
		return AVERROR(ENOMEM);
	return ff_set_common_formats(ctx, fmts_list);
}

//*************
#define OFFSET(x) offsetof(QNFilterContext, x)
#define VE AV_OPT_FLAG_VIDEO_PARAM|AV_OPT_FLAG_FILTERING_PARAM

static const AVOption qnfilter_options[] = {
	{ "enhtype",         "enhance type: lowlight_enh, deblock",          OFFSET(enh_type), AV_OPT_TYPE_STRING, {.str = ""}, .flags = VE},
	{ NULL }

};// TODO: add something if needed

static const AVClass qnfilter_class = {
	.class_name = "qnfilter",
	.item_name = av_default_item_name,
	.option = qnfilter_options,
	.version = LIBAVUTIL_VERSION_INT,
	.category = AV_CLASS_CATEGORY_FILTER,
};

static const AVFilterPad avfilter_vf_qnfilter_inputs[] = {
	{
		.name = "qnfilter_inputpad",
		.type = AVMEDIA_TYPE_VIDEO,
		.filter_frame = filter_frame,
	},
	{ NULL }
};

static const AVFilterPad avfilter_vf_qnfilter_outputs[] = {
	{
		.name = "qnfilter_outputpad",
		.type = AVMEDIA_TYPE_VIDEO,
		.config_props = config_output,
	},
	{ NULL }
};

AVFilter ff_vf_qnfilter = {
	.name = "qnfilter",
	.description = NULL_IF_CONFIG_SMALL("qiniu image filter"),
	.priv_size = sizeof(QNFilterContext),
	.priv_class = &qnfilter_class,
	.init = init,
	.uninit = uninit,
	.query_formats = query_formats,
	.inputs = avfilter_vf_qnfilter_inputs,
	.outputs = avfilter_vf_qnfilter_outputs,
};

