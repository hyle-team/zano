import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { OpenWalletModalComponent } from './open-wallet-modal.component';

describe('OpenWalletModalComponent', () => {
  let component: OpenWalletModalComponent;
  let fixture: ComponentFixture<OpenWalletModalComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ OpenWalletModalComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(OpenWalletModalComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
