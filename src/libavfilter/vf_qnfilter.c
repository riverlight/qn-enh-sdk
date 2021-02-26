#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavutil/avassert.h"
#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"

#include "qnfilter-sdk.h"


typedef struct QNFilterContext {
	const AVClass* class;

	const char* enh_type;
	QNFILTER_TYPE e_enh_type;
	Handle qnfilterHandle;
	unsigned char* i420buffer;

} QNFilterContext;

typedef struct ThreadData {
	AVFrame* in, * out;

} ThreadData;

static int avframe_2_I420buffer(AVFrame* in, unsigned char* buffer)
{
	unsigned char* y = buffer;
	unsigned char* u = buffer + in->width * in->height;
	unsigned char* v = buffer + in->width * in->height * 5 / 4;
	int i;
	for (i = 0; i < in->height; i++)
		memcpy(y + i * in->width, in->data[0] + in->linesize[0] * i, in->width);

	for (i = 0; i < in->height/2; i++)
	{
		memcpy(u + i * in->width / 2, in->data[1] + in->linesize[1] * i, in->width/2);
		memcpy(v + i * in->width / 2, in->data[2] + in->linesize[2] * i, in->width/2);
	}
	
	return 0;
}

static int I420buffer_2_avframe(unsigned char* buffer, AVFrame* out)
{
	unsigned char* y = buffer;
	unsigned char* u = buffer + out->width * out->height;
	unsigned char* v = buffer + out->width * out->height * 5 / 4;
	int i;
	for (i = 0; i < out->height; i++)
		memcpy(out->data[0] + out->linesize[0] * i, y + i * out->width, out->width);

	for (i = 0; i < out->height / 2; i++)
	{
		memcpy(out->data[1] + out->linesize[1] * i, u + i * out->width / 2, out->width / 2);
		memcpy(out->data[2] + out->linesize[2] * i, v + i * out->width / 2, out->width / 2);
	}

	return 0;
}

// core function
static int do_filter(AVFilterContext* ctx, void* arg, int jobnr, int nb_jobs)
{
//	av_log(NULL, AV_LOG_WARNING, "### Leon's QN filter_frame: core function \n");

	QNFilterContext* qnCtx = ctx->priv;
	ThreadData* td = arg;
	AVFrame* dst = td->out;
	AVFrame* src = td->in;

	avframe_2_I420buffer(src, qnCtx->i420buffer);
	QNFilter_Process_I420(qnCtx->qnfilterHandle, qnCtx->i420buffer, src->width, src->height);
	I420buffer_2_avframe(qnCtx->i420buffer, dst);

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
	QNFilterContext* qnCtx = ctx->priv;

	//you can modify output width/height here
	outlink->w = ctx->inputs[0]->w;
	outlink->h = ctx->inputs[0]->h;
	av_log(NULL, AV_LOG_DEBUG, "configure output, w h = (%d %d), format %d \n", outlink->w, outlink->h, outlink->format);

	if (strcmp(qnCtx->enh_type, "lowlight_enh")==0)
		qnCtx->e_enh_type = QF_LOWLIGHT;
	else if (strcmp(qnCtx->enh_type, "deblock")==0)
		qnCtx->e_enh_type = QF_DEBLOCK;
	else {
		av_log(qnCtx, AV_LOG_ERROR, "enhtype error [%s] \n", qnCtx->enh_type);
		return AVERROR(EINVAL);
	}

	qnCtx->qnfilterHandle = NULL;
	if (QNFilter_Create(&qnCtx->qnfilterHandle, qnCtx->e_enh_type) != 0) {
		av_log(qnCtx, AV_LOG_ERROR, "QNFilter_Create fail\n");
		return AVERROR(EINVAL);
	}

	qnCtx->i420buffer = (unsigned char*)malloc(outlink->w*outlink->h*3/2);
	if (qnCtx->i420buffer == NULL) {
		av_log(qnCtx, AV_LOG_ERROR, "malloc i420buffer fail\n");
		return AVERROR(EINVAL);
	}

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
	QNFilterContext* qnCtx = ctx->priv;
	//uninit something here if you want

	free(qnCtx->i420buffer);
	qnCtx->i420buffer = NULL;
	QNFilter_Destroy(qnCtx->qnfilterHandle);
	qnCtx->qnfilterHandle = NULL;
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

