import {Component, OnInit, Input, Output, EventEmitter, ViewChild, ElementRef} from '@angular/core';

@Component({
  selector: 'app-modal-container',
  templateUrl: './modal-container.component.html',
  styleUrls: ['./modal-container.component.scss']
})
export class ModalContainerComponent implements OnInit {

  public title: string;
  @Input() type: string;
  @Input() message: string;
  @Output() close = new EventEmitter<boolean>();
  @ViewChild('btn') button: ElementRef;

  constructor() {}

  ngOnInit() {
    this.button.nativeElement.focus();
    switch (this.type) {
      case 'error': this.title = 'Wrong'; break;
      case 'success': this.title = 'Success'; break;
      case 'info': this.title = 'Information'; break;
      default: this.title = 'Unexpected'; break;
    }
  }

  onClose() {
    this.close.emit();
  }
}
