import { Component, OnInit, Input, Output, EventEmitter, ViewChild, ElementRef } from '@angular/core';

@Component({
  selector: 'app-confirm-modal',
  templateUrl: './confirm-modal.component.html',
  styleUrls: ['./confirm-modal.component.scss']
})
export class ConfirmModalComponent implements OnInit {

<<<<<<< HEAD
<<<<<<< HEAD
  @Input() title: string;
=======
>>>>>>> confirm modal
=======
  @Input() title: string;
>>>>>>> fix conf. pop up, tooltip delay, rebuild html
  @Input() message: string;
  @Output() confirmed: EventEmitter<boolean> = new EventEmitter<boolean>();
  @ViewChild('btn') button: ElementRef;

  constructor() { }

  ngOnInit() {
    this.button.nativeElement.focus();
  }

  onSubmit() {
    this.confirmed.emit(true);
  }

  onClose() {
    this.confirmed.emit(false);
  }
}
