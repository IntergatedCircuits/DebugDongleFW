/**
  ******************************************************************************
  * @file    chrg_if.h
  * @author  Benedek Kupper
  * @version 1.0
  * @date    2018-03-10
  * @brief   Power Supply and Battery Charger HID interface header
  *
  * Copyright (c) 2018 Benedek Kupper
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
#ifndef CHRG_IF_H_
#define CHRG_IF_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <usbd_hid.h>
#include <chrg_ctrl.h>

extern USBD_HID_IfHandleType *const chrg_if;

void Charger_Periodic(void);

#ifdef __cplusplus
}
#endif

#endif /* CHRG_IF_H_ */
