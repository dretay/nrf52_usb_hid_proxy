#include "application.h"
#include "mouse_descriptor.h"

#define PIN_IN  3
#define LM75B_ADDR          (0x08)
#define TWI_INSTANCE_ID     0
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);
static volatile bool m_xfer_done = true;
#define BUFFER_SIZE 32
static uint8_t m_sample[BUFFER_SIZE];

#define INPUT_REP_BUTTONS_LEN           3                                           /**< Length of Mouse Input Report containing button data. */
#define INPUT_REP_MOVEMENT_LEN          3                                           /**< Length of Mouse Input Report containing movement data. */
#define INPUT_REP_BUTTONS_INDEX         0                                           /**< Index of Mouse Input Report containing button data. */
#define INPUT_REP_MOVEMENT_INDEX        1                                           /**< Index of Mouse Input Report containing movement data. */

static ble_hids_t *m_hids;
static uint16_t *m_conn_handle;
/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   x_delta   Horizontal movement.
 * @param[in]   y_delta   Vertical movement.
 */
static void mouse_movement_send(int16_t x_delta, int16_t y_delta)
{
	ret_code_t err_code;

	
	
	uint8_t buffer[INPUT_REP_MOVEMENT_LEN];

	APP_ERROR_CHECK_BOOL(INPUT_REP_MOVEMENT_LEN == 3);

	x_delta = MIN(x_delta, 0x0fff);
	y_delta = MIN(y_delta, 0x0fff);

	buffer[0] = x_delta & 0x00ff;
	buffer[1] = ((y_delta & 0x000f) << 4) | ((x_delta & 0x0f00) >> 8);
	buffer[2] = (y_delta & 0x0ff0) >> 4;

	err_code = ble_hids_inp_rep_send(m_hids,
		INPUT_REP_MOVEMENT_INDEX,
		INPUT_REP_MOVEMENT_LEN,
		buffer,
		*m_conn_handle);
	

	if ((err_code != NRF_SUCCESS) &&
	    (err_code != NRF_ERROR_INVALID_STATE) &&
	    (err_code != NRF_ERROR_RESOURCES) &&
	    (err_code != NRF_ERROR_BUSY) &&
	    (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
	{
		APP_ERROR_HANDLER(err_code);
	}
}
/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   index   the button index.
 * @param[in]   clicked  if the button has been clicked not.
 */
static void mouse_click_send(int16_t index, int16_t clicked)
{
	ret_code_t err_code;
	
	uint8_t buffer[INPUT_REP_BUTTONS_LEN];

	APP_ERROR_CHECK_BOOL(INPUT_REP_BUTTONS_LEN == 3);
	
	buffer[0] = clicked << index & 0x00ff;
	buffer[1] = 0;
	buffer[2] = 0;

	err_code = ble_hids_inp_rep_send(m_hids,
		INPUT_REP_BUTTONS_INDEX,
		INPUT_REP_BUTTONS_LEN,
		buffer,
		*m_conn_handle);
	

	if ((err_code != NRF_SUCCESS) &&
	    (err_code != NRF_ERROR_INVALID_STATE) &&
	    (err_code != NRF_ERROR_RESOURCES) &&
	    (err_code != NRF_ERROR_BUSY) &&
	    (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
	{
		APP_ERROR_HANDLER(err_code);
	}
}
void mouse_movement_event_handler(void *p_event_data, uint16_t event_size)
{
	// In this case, p_event_data is a pointer to a nrf_drv_gpiote_pin_t that represents
	// the pin number of the button pressed. The size is constant, so it is ignored.
	mouse_movement_send(((MouseUpdate*)p_event_data)->x, ((MouseUpdate*)p_event_data)->y);	
}
void mouse_click_event_handler(void *p_event_data, uint16_t event_size)
{
	// In this case, p_event_data is a pointer to a nrf_drv_gpiote_pin_t that represents
	// the pin number of the button pressed. The size is constant, so it is ignored.
	mouse_click_send(((MouseUpdate*)p_event_data)->type, ((MouseUpdate*)p_event_data)->x);	
}
void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	if (m_xfer_done == true)
	{
		m_xfer_done = false;
		ret_code_t err_code = nrf_drv_twi_rx(&m_twi, LM75B_ADDR, m_sample, sizeof(m_sample));
		APP_ERROR_CHECK(err_code);
	}	
}
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	switch (p_event->type)
	{
	case NRF_DRV_TWI_EVT_DONE:
		if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
		{
			NRF_LOG_INFO("rx something!");
			//     /* Allocate space for the decoded message. */
			MouseUpdate mouse_update = MouseUpdate_init_zero;
    
			/* Create a stream that reads from the buffer. */
			pb_istream_t decode_stream = pb_istream_from_buffer(m_sample, BUFFER_SIZE);
    
			/* Now we are ready to decode the message. */
			bool status = pb_decode_delimited(&decode_stream, MouseUpdate_fields, &mouse_update);
						
			if (status == true) 
			{				
				switch (mouse_update.type)
				{
				case MouseUpdate_Type_XY:
					NRF_LOG_INFO("dx= %d dy= %d", mouse_update.x, mouse_update.y);      
					app_sched_event_put(&mouse_update, sizeof(mouse_update), mouse_movement_event_handler);
					break;
				case MouseUpdate_Type_LEFT:
					NRF_LOG_ERROR("Left click");   
					app_sched_event_put(&mouse_update, sizeof(mouse_update), mouse_click_event_handler);
					break;
				case MouseUpdate_Type_RIGHT:
					NRF_LOG_ERROR("Right click");   
					app_sched_event_put(&mouse_update, sizeof(mouse_update), mouse_click_event_handler);
					break;
				case MouseUpdate_Type_MIDDLE:
					NRF_LOG_ERROR("Middle click");   
					app_sched_event_put(&mouse_update, sizeof(mouse_update), mouse_click_event_handler);
					break;
				default:
					NRF_LOG_ERROR("Unrecognized / unsuppored mouse update");   
				}				
			}
			else {      
				NRF_LOG_ERROR("Decode unsuccessful");   
			}  
		}		
		break;
	default:
		break;
	}
	m_xfer_done = true;
}
/**@brief Function for handling HID events.
 *
 * @details This function will be called for all HID events which are passed to the application.
 *
 * @param[in]   p_hids  HID service structure.
 * @param[in]   p_evt   Event received from the HID service.
 */
static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt)
{
	switch (p_evt->evt_type)
	{
	case BLE_HIDS_EVT_BOOT_MODE_ENTERED:		
		break;

	case BLE_HIDS_EVT_REPORT_MODE_ENTERED:
		
		break;

	case BLE_HIDS_EVT_NOTIF_ENABLED:
		break;

	default:
		// No implementation needed.
		break;
	}
}
/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
}
/**@brief Function for initializing HID Service.
 */
static void hids_init(void)
{
	ret_code_t                err_code;
	ble_hids_init_t           hids_init_obj;
	ble_hids_inp_rep_init_t * p_input_report;
	uint8_t                   hid_info_flags;

	static ble_hids_inp_rep_init_t inp_rep_array[INPUT_REPORT_COUNT];

	memset(inp_rep_array, 0, sizeof(inp_rep_array));
	// Initialize HID Service.
	p_input_report                      = &inp_rep_array[INPUT_REP_BUTTONS_INDEX];
	p_input_report->max_len             = INPUT_REP_BUTTONS_LEN;
	p_input_report->rep_ref.report_id   = INPUT_REP_REF_BUTTONS_ID;
	p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

	p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
	p_input_report->sec.wr      = SEC_JUST_WORKS;
	p_input_report->sec.rd      = SEC_JUST_WORKS;

	p_input_report                      = &inp_rep_array[INPUT_REP_MOVEMENT_INDEX];
	p_input_report->max_len             = INPUT_REP_MOVEMENT_LEN;
	p_input_report->rep_ref.report_id   = INPUT_REP_REF_MOVEMENT_ID;
	p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

	p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
	p_input_report->sec.wr      = SEC_JUST_WORKS;
	p_input_report->sec.rd      = SEC_JUST_WORKS;

	p_input_report                      = &inp_rep_array[INPUT_REP_MPLAYER_INDEX];
	p_input_report->max_len             = INPUT_REP_MEDIA_PLAYER_LEN;
	p_input_report->rep_ref.report_id   = INPUT_REP_REF_MPLAYER_ID;
	p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

	p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
	p_input_report->sec.wr      = SEC_JUST_WORKS;
	p_input_report->sec.rd      = SEC_JUST_WORKS;

	hid_info_flags = HID_INFO_FLAG_REMOTE_WAKE_MSK | HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK;

	memset(&hids_init_obj, 0, sizeof(hids_init_obj));

	hids_init_obj.evt_handler                    = on_hids_evt;
	hids_init_obj.error_handler                  = service_error_handler;
	hids_init_obj.is_kb                          = false;
	hids_init_obj.is_mouse                       = true;
	hids_init_obj.inp_rep_count                  = INPUT_REPORT_COUNT;
	hids_init_obj.p_inp_rep_array                = inp_rep_array;
	hids_init_obj.outp_rep_count                 = 0;
	hids_init_obj.p_outp_rep_array               = NULL;
	hids_init_obj.feature_rep_count              = 0;
	hids_init_obj.p_feature_rep_array            = NULL;
	hids_init_obj.rep_map.data_len               = sizeof(ReportDescriptor);
	hids_init_obj.rep_map.p_data                 = ReportDescriptor;
	hids_init_obj.hid_information.bcd_hid        = BASE_USB_HID_SPEC_VERSION;
	hids_init_obj.hid_information.b_country_code = 0;
	hids_init_obj.hid_information.flags          = hid_info_flags;
	hids_init_obj.included_services_count        = 0;
	hids_init_obj.p_included_services_array      = NULL;

	hids_init_obj.rep_map.rd_sec         = SEC_JUST_WORKS;
	hids_init_obj.hid_information.rd_sec = SEC_JUST_WORKS;

	hids_init_obj.boot_mouse_inp_rep_sec.cccd_wr = SEC_JUST_WORKS;
	hids_init_obj.boot_mouse_inp_rep_sec.wr      = SEC_JUST_WORKS;
	hids_init_obj.boot_mouse_inp_rep_sec.rd      = SEC_JUST_WORKS;

	hids_init_obj.protocol_mode_rd_sec = SEC_JUST_WORKS;
	hids_init_obj.protocol_mode_wr_sec = SEC_JUST_WORKS;
	hids_init_obj.ctrl_point_wr_sec    = SEC_JUST_WORKS;

	err_code = ble_hids_init(m_hids, &hids_init_obj);
	APP_ERROR_CHECK(err_code);
}
static void run()
{
	ret_code_t err_code;
	const nrf_drv_twi_config_t twi_lm75b_config = {
		.scl = ARDUINO_SCL_PIN,
		.sda = ARDUINO_SDA_PIN,
		.frequency = NRF_DRV_TWI_FREQ_400K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init = false
	};

	err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, twi_handler, NULL);
	APP_ERROR_CHECK(err_code);
	nrf_drv_twi_enable(&m_twi);

	nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
	in_config.pull = NRF_GPIO_PIN_PULLUP;

	err_code = nrf_drv_gpiote_in_init(PIN_IN, &in_config, in_pin_handler);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_event_enable(PIN_IN, true);
}
static void init(ble_hids_t *m_hids_in, uint16_t *m_conn_handle_in)
{
	m_hids = m_hids_in;
	m_conn_handle = m_conn_handle_in;

	hids_init();
}

const struct application Application = { 
	.init = init,
	.run = run,		
};