import { Component, OnInit, Output, EventEmitter, Input} from '@angular/core';
import {FormControl, FormGroup, Validators} from '@angular/forms';
import { VariablesService } from '../_helpers/services/variables.service';


@Component({
  selector: 'app-send-modal',
  templateUrl: './send-modal.component.html',
  styleUrls: ['./send-modal.component.scss']
})
export class SendModalComponent implements OnInit {

  confirmForm = new FormGroup({
    password: new FormControl('')
  });

  @Input() form: FormGroup;
  @Output() confirmed: EventEmitter<boolean> = new EventEmitter<boolean>();

  constructor(
    public variablesService: VariablesService,
  ) {
  }

  ngOnInit() {
    if(this.variablesService.appPass) {
      this.confirmForm.controls['password'].setValidators([Validators.required]);
      this.confirmForm.updateValueAndValidity();
    }
  }

  confirm() {
    if(this.variablesService.appPass) {
      if (this.confirmForm.controls['password'].value === '') {
        this.confirmForm.controls['password'].setErrors({requiredPass: true});
        return;
      }
      this.confirmForm.controls['password'].setErrors({requiredPass: false});
      if (this.variablesService.appPass ===  this.confirmForm.controls['password'].value) {
        this.confirmed.emit(true);
      } else {
        this.confirmForm.controls['password'].setErrors({passwordNotMatch: true})
      }
    } else {
      this.confirmed.emit(true);
    }
  }

  onClose() {
    this.confirmed.emit(false);
  }

}
