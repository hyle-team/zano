import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { SendModalComponent } from './send-modal.component';

describe('SendModalComponent', () => {
  let component: SendModalComponent;
  let fixture: ComponentFixture<SendModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ SendModalComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SendModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
