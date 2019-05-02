import {Component, OnInit, Input, Output, EventEmitter, ViewChild, ElementRef} from '@angular/core';
import {TranslateService} from '@ngx-translate/core';

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

  constructor(private translate: TranslateService) {}

  ngOnInit() {
    this.button.nativeElement.focus();
    switch (this.type) {
      case 'error': this.title = this.translate.instant('MODALS.ERROR'); break;
      case 'success': this.title = this.translate.instant('MODALS.SUCCESS'); break;
      case 'info': this.title = this.translate.instant('MODALS.INFO'); break;
    }
  }

  onClose() {
    this.close.emit();
  }
}
