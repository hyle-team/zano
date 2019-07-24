import { Component, OnInit, Output, EventEmitter, Input } from '@angular/core';
import { FormGroup } from '@angular/forms';
import { VariablesService } from '../_helpers/services/variables.service';


@Component({
  selector: 'app-send-modal',
  templateUrl: './send-modal.component.html',
  styleUrls: ['./send-modal.component.scss']
})
export class SendModalComponent implements OnInit {

  @Input() form: FormGroup;
  @Output() confirmed: EventEmitter<boolean> = new EventEmitter<boolean>();

  constructor(
    public variablesService: VariablesService
  ) {
  }

  ngOnInit() {
  }

  confirm() {
    this.confirmed.emit(true);
  }

  onClose() {
    this.confirmed.emit(false);
  }

}
