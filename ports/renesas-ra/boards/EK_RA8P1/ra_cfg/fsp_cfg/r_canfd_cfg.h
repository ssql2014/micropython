/* generated configuration header file - do not edit */
#ifndef R_CANFD_CFG_H_
#define R_CANFD_CFG_H_
#ifdef __cplusplus
extern "C" {
#endif

#define CANFD_CFG_PARAM_CHECKING_ENABLE  (BSP_CFG_PARAM_CHECKING_ENABLE)

/* AFL rule counts per channel.  We use 1 accept-all rule on ch0, none on ch1. */
#define CANFD_CFG_AFL_CH0_RULE_NUM       (1)
#define CANFD_CFG_AFL_CH1_RULE_NUM       (0)

/* FD protocol exception: 0 = disabled (recommended for most embedded use). */
#define CANFD_CFG_FD_PROTOCOL_EXCEPTION  (0)

/* Channel that handles global error interrupts (ch 0 on single-instance boards). */
#define CANFD_CFG_GLOBAL_ERROR_CH        (0)

#ifdef __cplusplus
}
#endif
#endif /* R_CANFD_CFG_H_ */
